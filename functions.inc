<?php
/**
 * @file
 * Defines the functinos
 *
 * @ingroup urls-monitor
 * @{
 */
global $conf;
$conf = parse_ini_file('config.ini');
 /**
 * Return an array of domains
 *
 * @return array
 */
function urls_monitor_urls() {
  $filename = 'urls.txt';
  $domains = array();
  if (is_readable($filename)) {
    $domains = file_get_contents($filename);
    $domains = explode("\n", trim($domains));
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
  if (array_key_exists($raw, $conf['alias'])) {
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
 */
function urls_monitor_check($host) {
  global $conf;
  $result = array(
    'host' => $host,
    'url' => $host,
  );
  if (!strstr($host, ':/')) {
    $result['url'] = 'http://' . $host;
  }
  if (!($parsed = parse_url($result['url']))) {
    return;
  }

  //@todo Make this to see response code and detect redirect
  $timeout = empty($conf['timeout']) ? 10 : $conf['timeout'];

  $curl = curl_init($result['host']);
  curl_setopt($curl, CURLOPT_CONNECTTIMEOUT, $timeout);
  curl_setopt($curl, CURLOPT_TIMEOUT, $timeout);
  curl_setopt($curl, CURLOPT_NOBODY, TRUE);
  curl_setopt($curl, CURLOPT_HEADER, TRUE);
  curl_setopt($curl, CURLOPT_FOLLOWLOCATION, TRUE);
  curl_setopt($curl, CURLOPT_FRESH_CONNECT, TRUE);
  curl_setopt($curl, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($curl, CURLOPT_AUTOREFERER, TRUE);

  $data = curl_exec($curl);
  $info = curl_getinfo($curl);
  curl_close($curl);

  // Determine if this redirects
  $result['redirect'] = NULL;
  if ($info['redirect_count'] > 1) {
    $redirect = parse_url($info['url']);
    $result['redirect'] = $redirect['host'];
  }

  $result['status'] = empty($conf['status'][$info['http_code']]) ? $info['http_code'] : $conf['status'][$info['http_code']];
  $result['ip'] = NULL;
  if ($ip = gethostbyname($result['host'])) {
    $result['ip'] = $ip;
  }
  return $result;
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
    'title' => empty($conf['page_title']) ? 'Website Status' : $conf['page_title'],
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

/**
 * Trim the leading www. from an url
 *
 * @param string &$url
 *
 * @return NULL
 */
function urls_monitor_www_trim(&$url) {
  if (($temp = parse_url($url)) && !empty($temp['host'])) {
    $url = $temp['host'];
  }
  if (substr($url, 0, 4) == 'www.') {
    $url = substr($url, 4);
  }
}

/** @} */ //end of group urls-monitor
