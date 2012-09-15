<?php
/**
 * @file
 * Defines the functinos
 *
 * @ingroup urls-monitor
 * @{
 */

 /**
 * Return an array of domains
 *
 * @return array
 */
function get_domains() {
  $filename = 'domains.txt';
  $domains = array();
  if (is_readable($filename)) {
    $domains = file_get_contents($filename);
    $domains = explode("\n", trim($domains));
  }
  return $domains;
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
 */
function ping($host, $timeout = 6) {
  $result = array(
    'host' => $host,
  );
  if (!strstr($host, ':/')) {
    $host = 'http://' . $host;
  }
  if (!($parsed = parse_url($host))) {
    return;
  }
  $info = array_filter($parsed) + array(
    'port' => 80,
  );

  //@todo Make this to see response code and detect redirect
  $fp = @fsockopen($info['host'], $info['port'], $errno, $errstr, $timeout);
  $result['status'] = (bool) $fp;
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
function theme_table($rows, $id = '', $header = array()) {
  $output = '';
  $output .= '<table id="'. $id .'" cellspacing="0" cellpadding="0">'."\n";

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
function page($info) {
  $output = <<<EOD
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>{$info['title']}</title>
    <meta name="description" content="Results of the domain monitor">
    <link rel="stylesheet" href="style.css">
  </head>
  <body>
    <h1>{$info['title']}</h1>
    {$info['body']}
  </body>
</html>
EOD;
  return $output;
}


/** @} */ //end of group urls-monitor
