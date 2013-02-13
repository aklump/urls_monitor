<?php
/**
 * @file
 * Defines the functinos
 *
 * @ingroup urls-monitor
 * @{
 */
global $conf;

if (!file_exists('config.ini')) {
  fatal('Please create config.ini and try again.');
}

$conf = parse_ini_file('config.ini');
$conf['columns'] = array_filter($conf['columns']);

/**
 * Return an array of domains
 *
 * @return array
 */
function urls_monitor_urls() {
  static $domains = array();
  if (empty($domains)) {
    $filename = 'urls.txt';
    $domains = array();
    if (is_readable($filename) &&
        ($list = trim(file_get_contents($filename)))) {
      $domains = explode("\n", trim($list));
      // Remove blank lines
      $domains = array_filter($domains);
      // Remove commented lines
      $domains = array_filter($domains, create_function('$url', 'return !preg_match("/^(;|#|\/\/)/", $url);'));
    }
  }
  return $domains;
}

/**
 * Translate a string
 *
 * @param string $raw
 *
 * @return string
 *   The translated string or the raw string
 */
function urls_monitor_alias($raw) {
  global $conf;
  if (is_array($conf['alias']) &&
      array_key_exists($raw, $conf['alias'])) {
    return $conf['alias'][$raw];
  }
  return $raw;
}

/**
 * Return an status array for a domain
 *
 * @param string $host
 * @param int $timeout
 *
 * @return array
 *   - host
 *   - status (bool)
 *   - ip
 *
 * @todo figure out why the session doesn't pass in the curl calls
 */
function urls_monitor_check($host) {
  global $conf;

  if (!($parsed = urls_monitor_parse_url($host))) {
    return;
  }

  // juggle this around so the column order is based on config.ini
  $result = array_fill_keys(array_keys($conf['columns']), NULL);
  foreach ($parsed as $key => $value) {
    $result[$key] = $value;
  }
  $result['status'] = NULL;

  //@todo Make this to see response code and detect redirect
  $timeout = empty($conf['timeout']) ? 10 : $conf['timeout'];

  // are we looking for content on this page?
  $find = !empty($conf['find'][$result['host']]);

  $curl = curl_init($result['host']);
  curl_setopt($curl, CURLOPT_CONNECTTIMEOUT, $timeout);
  curl_setopt($curl, CURLOPT_TIMEOUT, $timeout);
  curl_setopt($curl, CURLOPT_NOBODY, !$find);
  curl_setopt($curl, CURLOPT_HEADER, TRUE);
  curl_setopt($curl, CURLOPT_FOLLOWLOCATION, TRUE);
  curl_setopt($curl, CURLOPT_FRESH_CONNECT, TRUE);
  curl_setopt($curl, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($curl, CURLOPT_AUTOREFERER, TRUE);

  $userpwd = FALSE;
  $result['password'] = '-';
  if (array_key_exists('httpauth', $conf)) {
    if (!empty($conf['httpauth'][$host])) {
      $userpwd = $conf['httpauth'][$host];
    }
    else {
      foreach ($conf['httpauth'] as $regex => $value) {
        $regex = trim($regex, '/');
        if (preg_match('/' . $regex . '/', $host)) {
          $userpwd = $value;
          break;
        }
      }
    }

    if ($userpwd) {

      // Make a call and make sure its 401 first
      $data = curl_exec($curl);
      $info = curl_getinfo($curl);
      if (!($lock_test = $info['http_code'] == 401)) {
        $result['password'] = 'missing';
      }
      else {
        $result['password'] = 'locked';
      }

      // Now set the credentials provided
      curl_setopt($curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
      curl_setopt($curl, CURLOPT_USERPWD, $userpwd);
    }
  }

  $data = curl_exec($curl);
  $info = curl_getinfo($curl);
  curl_close($curl);

  if ($conf['redirect_count']) {
    $result['redirects'] = $info['redirect_count'];
  }

  // Determine if this redirects
  $result['redirected'] = NULL;
  $redirect = urls_monitor_parse_url($info['url']);
  if (strcasecmp($info['url'], $result['url'])) {
    $result['redirected'] = $redirect['host'];
    if ($conf['redirect_full_path'] &&  !empty($redirect['path'])) {
      $result['redirected'] .= $redirect['path'];
    }
  }

  $result['status'] = $info['http_code'];

  // Password status
  if ($userpwd
      && !in_array($result['password'], array('missing', 'no', '-'))
      && $result['status'] != 401) {
    $result['password'] = 'un-locked';
  }

  // Only look for string if we haven't any errors
  $result['text'] = '-';
  if ($result['status'] && $result['status'] < 300 && $find) {
    $result['status'] = 2;
    $haystack = substr($data, $info['header_size']);
    $needles = $conf['find'][$result['host']];

    //@todo there is no support yet in the configuration to make this an array;
    //come up with one
    if (!is_array($needles)) {
      $needles = array($needles);
    }
    $found = 0;
    // performs and and search
    foreach ($needles as $needle) {
      if (strstr($haystack, $needle)) {
        $found++;
      }
    }
    $result['text'] = 'missing';
    if ($found == count($needles)) {
      $result['status'] = $info['http_code'];
      $result['text'] = 'ok';
    }
  }

  $result['ip'] = NULL;
  if ($ip = gethostbyname($result['host'])) {
    $result['ip'] = $ip;
  }

  // set status for a missing lock
  if ($result['password'] == 'missing') {
    $result['status'] = 1;
  }

  return $result;
}

/**
 * Parse an url
 *
 * @param string $url
 *
 * @return array
 *   - schema
 *   - host
 */
function urls_monitor_parse_url($url) {
  $url = strtolower(trim($url, '/'));
  if (!strstr($url, ':/')) {
    $url = 'http://' . $url;
  }
  $parsed = parse_url($url) + array(
    'url' => $url,
  );
  unset($parsed['scheme']);
  return $parsed;
}

/**
 * Theme a table
 *
 * @param $rows array
 *   An array of rows, each an array of cells
 *
 * @return string
 *   The html for a table
 */
function urls_monitor_theme_table($rows, $attributes, $header = array()) {
  $output = '';

  $attr = array();
  foreach ($attributes as $key => $value) {
    $attr[] = "$key=\"$value\"";
  }
  $attr = ' ' . implode(' ', $attr);
  $output .= '<table' . $attr . ' cellspacing="0" cellpadding="0">'."\n";
  if (!empty($header)) {
    $output .= "<thead><tr>\n<th>" . implode("</th>\n<th>", $header) . "</th>\n</tr></thead>\n";
  }

  $alt = 0;
  foreach ($rows as $cells) {
    if (!isset($cells['data'])) {
      $cells['data'] = $cells;
    }
    $class = '';
    if (!empty($cells['class'])) {
      $class .= implode(' ', $cells['class']);
    }
    $class .= ' '. ($alt ? 'even' : 'odd');
    $alt = !$alt;
    $output .= '<tr class="'. trim($class) .'">'."\n";
    $id = 1;
    foreach ($cells['data'] as $cell) {
      $output .= '<td class="col-'. $id++ .'">'. $cell .'</td>'."\n";
    }
    $output .= '</tr>'."\n";
  }
  $output .= '</table>'."\n";
  return $output;
}

/**
 * Wrap the output with an html page
 *
 * @param array
 * - title
 * - body
 *
 * @return string
 */
function urls_monitor_page($info) {
  global $conf;
  $info += array(
    'title' => empty($conf['page_title']) ? 'Websites Status' : $conf['page_title'],
    'subtitle' => ($count = count(urls_monitor_urls())) == 1 ? '1 Domain' : "$count Domains" . ' Monitored',
  );
  $output = <<<EOD
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>{$info['title']}</title>
    <meta name="description" content="Results of the domain monitor">
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="custom.css">
    <script src="//ajax.googleapis.com/ajax/libs/jquery/1.8.0/jquery.min.js" type="text/javascript"></script>
    <script type="text/javascript" src="jquery.tablesorter.min.js"></script>
    <script>
      $(document).ready(function()
          {
              $("#monitor-results").tablesorter();
          }
      );
    </script>
  </head>
  <body>
    <h1>{$info['title']}</h1>
    <h2>{$info['subtitle']}</h2>
    {$info['body']}
  </body>
</html>
EOD;
  return $output;
}


/**
 * Make a css safe class or id
 *
 * @param string or array $input
 *   The string will be made css safe; array values will be made css safe
 * @param bool $flatten
 *   Optional.  Set to TRUE to flatten arrays into space separated strings
 *
 * @return string or array
 */
function urls_monitor_css_safe($input, $flatten = FALSE) {
  if (is_array($input)) {
    $function = __FUNCTION__;
    foreach ($input as $key => $value) {
      $input[$key] = $function($value);
    }
    return $flatten ? trim(implode(' ', $input)) : $input;
  }
  elseif (is_string($input)) {
    $input = trim(strtolower(preg_replace('/[\W_]+/', '-', $input)), '-');
    if (preg_match('/^(\d)(.*)/', $input, $found)) {
      $word = array('zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine');
      $input = $word[$found[1]] . $found[2];
    }
  }
  return $input;
}

function fatal($message) {
  $message = '<div class="message fatal">' . $message . '</div>';
  print urls_monitor_page(array('body' => $message));
  exit;
}

/** @} */ //end of group urls-monitor
