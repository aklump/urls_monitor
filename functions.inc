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
$conf += array(
  'alias' => array(),
  'status' => array(),
  'columns' => array(),
  'find' => array(),
  'httpauth' => array(),
);
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
  if (!empty($conf['alias'])
      && is_array($conf['alias']) &&
      array_key_exists($raw, $conf['alias'])) {
    return $conf['alias'][$raw];
  }
  return $raw;
}

/**
 * Return an status array for a domain
 *
 * @param string $host
 * @param bool $skip_curl
 *   Set to true to skip the actual curl call
 *
 * @return array
 *   - host
 *   - status (bool)
 *   - ip
 *
 * @todo figure out why the session doesn't pass in the curl calls
 */
function urls_monitor_check($host, $skip_curl = FALSE) {
  global $conf;

  if (!($parsed = urls_monitor_parse_url($host))) {
    return;
  }

  // juggle this around so the column order is based on config.ini
  $result = array_fill_keys(array_keys($conf['columns']), NULL);
  foreach ($parsed as $key => $value) {
    $result[$key] = $value;
  }
  $not_checked = '-';
  $result['status']       = $not_checked;
  $result['password']     = $not_checked;
  $result['redirects']    = $not_checked;
  $result['redirected']   = $not_checked;
  $result['ip']           = $not_checked;

  if ($skip_curl) {
    return $result;
  }

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
 * Preprocess the row data
 *
 * @param array &$row
 *   See example for exptected format
 *
 * @code
 *   $row = array(
 *     'data' => urls_monitor_check($domain),
 *   );
 * @endcod
 *
 * @return NULL
 *
 * @see urls_monitor_check()
 *
 */
function urls_monitor_preprocess_row(&$row) {
  global $conf;

  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['ip']));

  // Ajax reload
  $row['data']['check'] = array(
    'data' => '<a href="#" onclick="return false;" class="ajax-check" rel="' . $row['data']['host'] . '"><img src="images/reload.gif" alt="ajax-reload-icon" /></a>',
    'class' => array('ajax'),
  );

  // Status
  $row['data']['status'] = empty($conf['status'][$row['data']['status']]) ?
    $row['data']['status'] :
    $conf['status'][$row['data']['status']];
  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['status']));

  // Add a link to the website
  $display = $row['data']['host'];

  // Link to website
  $row['data']['host'] = $display . ' <a class="external-link" href="' . $row['data']['url'] . '" onclick="window.open(this.href); return false;"><img src="images/external.png" /></a>';
  unset($row['data']['url']);

  // Link to the actual ip if different from alias
  if (($alias = urls_monitor_alias($row['data']['ip'])) && $alias != $row['data']['ip']) {
    $row['data']['ip'] = $alias . ' <a class="external-link" href="javascript:alert(\'' . $row['data']['ip'] . '\'); return false;" title="' . $row['data']['ip'] . '"><img src="images/external.png" /></a>';
  }

  // We have to have the check column set to true or ajax won't work
  $conf['columns'] += array(
    'check' => TRUE,
  );
  $conf['columns'] = TRUE;
  $row['data'] = array_intersect_key($row['data'], $conf['columns']);
}

/**
 * Theme a table
 *
 * @param $rows array
 *   An array of rows, each an array of cells
 *   Each row is an array with:
 *   - data: an array of cells
 *   - class: an array of classes for tr
 *     Each cell array is either a string or an array
 *     - data: the cell contents
 *     - class: an array of classes for td
 * @param array $attributes
 *   An array of attributes to apply to the table
 *
 * @return string
 *   The html for a table
 */
function urls_monitor_theme_table($rows, $attributes = array(), $header = array()) {
  $output = '';

  $attr = array();
  $attributes += array(
    'cellspacing' => 0,
    'cellpadding' => 0,
  );
  foreach ($attributes as $key => $value) {
    $attr[] = "$key=\"$value\"";
  }
  $attr = ' ' . implode(' ', $attr);
  $output .= '<table' . $attr . '>'."\n";
  $output .= '<thead><tr>' . "\n";
  $class = '';
  if (!empty($header)) {
    foreach (array_keys($header) as $key) {
      $th = array();
      if (!is_array($header[$key])) {
        $th['data'] = $header[$key];
      }
      else {
        $th = $header[$key];
      }
      if (!empty($th['class'])) {
        $class = implode(' ', (array) $th['class']);
      }
      $output .= '<th class="'. trim($class) .'">' . $th['data'] . '</th>'."\n";
    }
    $output .= "</tr></thead>\n";
  }

  $alt = 0;
  foreach (array_keys($rows) as $row_key) {
    $output .= urls_monitor_theme_tr($rows[$row_key], $alt) . "\n";
  }
  $output .= '</table>'."\n";
  return $output;
}

/**
 * Theme a row
 */
function urls_monitor_theme_tr($row_data, $alt = 0) {
  $output = '';
  $row = array();
  if (!isset($row_data['data'])) {
    $row['data'] = $row_data;
  }
  else {
    $row = $row_data;
  }
  $row['class'][] = ($alt ? 'even' : 'odd');
  $alt = !$alt;
  $class = implode(' ', (array) $row['class']);
  $output .= '<tr class="'. trim($class) .'">'."\n";
  $id = 1;
  foreach (array_keys($row['data']) as $cell_key) {
    if (!is_array($row['data'][$cell_key])) {
      $cell = array('data' => $row['data'][$cell_key]);
    }
    else {
      $cell = $row['data'][$cell_key];
    }
    $cell['class'][] = ' col-' . $id++;
    $class = implode(' ', (array) $cell['class']);
    $output .= '<td class="' . trim($class) . '">'. $cell['data'] .'</td>'."\n";
  }
  $output .= '</tr>'."\n";
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
    <script type="text/javascript" src="urls_monitor.js"></script>
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
    <div id="loading">Checking Now...</div>
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
