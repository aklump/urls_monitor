<?php
/**
 * @file
 * Defines the functinos
 *
 * @ingroup urls-monitor
 * @{
 */
/**
 * @var URLS_MONITOR_BLANK
 *
 * String value to use on NULL or unknown cell values
 */
define('URLS_MONITOR_BLANK', '-');

global $conf;
$config_file = urls_monitor_get_config_dir() . '/config.ini';
if (!file_exists($config_file)) {
  urls_monitor_fatal('Please create config.ini and try again.');
}

$conf = parse_ini_file($config_file);
$conf += array(
  'alias' => array(),
  'status' => array(),
  'columns' => array(),
  'find' => array(),
  'httpauth' => array(),
);
$conf['columns'] = array_filter($conf['columns']);

/**
 * Get the current page
 */
function urls_monitor_get_page() {
  return empty($_GET['q']) ? 'default' : $_GET['q'];
}

/**
 * Get the current config directory
 */
function urls_monitor_get_config_dir() {
  return getcwd() . '/config/' . urls_monitor_get_page();
}

/**
 * Return an array of valid pages
 */
function urls_monitor_get_pages() {
  $pages = array();
  $directory = 'config';
  $recursive = TRUE;
  $files = urls_monitor_list_files($directory, $recursive);
  foreach ($files as $file) {
    if (substr($file, -11) == '/config.ini') {
      if (($page = substr($file, 7, -11))) {
        $pages[$page] = $page;
      }
    }
  }
  asort($pages);
  return $pages;
}

function urls_monitor_list_files($directory, $recursive) {
  $array_items = array();
  if ($handle = opendir($directory)) {
    while (false !== ($file = readdir($handle))) {
      if ($file != "." && $file != "..") {
        if (is_dir($directory. "/" . $file)) {
          if($recursive) {
            $array_items = array_merge($array_items, urls_monitor_list_files($directory. "/" . $file, $recursive));
          }
          $file = $directory . "/" . $file;
          $array_items[] = preg_replace("/\/\//si", "/", $file);
          }
          else {
            $file = $directory . "/" . $file;
            $array_items[] = preg_replace("/\/\//si", "/", $file);
          }
        }
      }
      closedir($handle);
    }
  return $array_items;
}

/**
 * Return an array of domains
 *
 * @return array
 */
function urls_monitor_urls() {
  static $domains = array();
  if (empty($domains)) {
    $filename = urls_monitor_get_config_dir() . '/urls.txt';
    $domains = array();
    if (is_readable($filename) &&
        ($domains = file($filename))) {
      // Remove blank lines
      $domains = array_filter($domains);
      foreach (array_keys($domains) as $key) {
        $domains[$key] = trim($domains[$key]);
        // Remove commented lines
        if (empty($domains[$key])
            || preg_match("/^(;|#|\/\/)/", ($domains[$key]))) {
          unset($domains[$key]);
        }
      }
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
      && is_array($conf['alias'])) {
    // Look for exact match
    if (array_key_exists($raw, $conf['alias'])) {
      return $conf['alias'][$raw];
    }
    // Look for wildcard
    foreach (array_keys($conf['alias']) as $key) {
      if (substr($raw, 0, strlen($key)) == $key) {
        return $conf['alias'][$key];
      }
    }
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
  $result['status']       = URLS_MONITOR_BLANK;
  $result['text']         = URLS_MONITOR_BLANK;
  $result['password']     = URLS_MONITOR_BLANK;
  $result['redirects']    = URLS_MONITOR_BLANK;
  $result['redirected']   = URLS_MONITOR_BLANK;
  $result['ip']           = URLS_MONITOR_BLANK;
  $result['ttl']          = URLS_MONITOR_BLANK;

  if ($skip_curl) {
    return $result;
  }

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
        if (substr($value, 0, 1) === '/') {
          $regex = trim($regex, '/');
          if (preg_match('/' . $regex . '/', $host)) {
            $userpwd = $value;
            break;
          }
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

  // TTL using dig
  if (($dig = urls_monitor_parse_dig($result['host']))
      && !empty($dig['ttl'])) {
    $result['ttl'] = $dig['ttl'];
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
  if (($parsed = parse_url($url))) {
    $parsed += array(
      'url' => $url,
    );
    unset($parsed['scheme']);
  }
  return $parsed;
}

/**
 * Return parsed dig results for a domain
 *
 * @param string $url
 *
 * @return array
 *   tbd
 */
function urls_monitor_parse_dig($url) {
  static $dig = NULL;
  if ($dig === NULL) {
    exec('which dig', $dig);
    $dig = array_shift($dig);
  }
  if (!$dig) {
    return array();
  }
  $parsed = array();

  // Get the authoritative nameserver
  $cmd = $dig . ' +nssearch ' . $url;
  exec($cmd, $output, $return);
  if (preg_match('/(?:\d{1,3}\.){3}\d{1,3}/', end($output), $found)) {
    $auth_ns = $found[0];
  }

  // Get the TTL
  $cmd = $dig . ' +noauthority +noquestion +nostats @' . $auth_ns . ' ' . $url;
  exec($cmd, $output, $return);
  $answer = FALSE;
  foreach ($output as $key => $value) {
    if (stristr($value, 'answer section')) {
      $answer = TRUE;
      continue;
    }
    if ($answer
        && preg_match('/(\d+)\sIN\sA\s([\d\.]+)$/', $value, $parsed)) {
      $parsed = array(
        'ttl' => $parsed[1],
        'ip' => $parsed[2],
      );
      break;
    }
  }

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

  $host = $row['data']['host'];

  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['ip']));

  // Ajax reload
  $row['data']['check'] = array(
    'data' => '<a href="#" onclick="return false;" class="ajax-check" rel="' . $host . '"><img src="/images/reload.gif" alt="ajax-reload-icon" /></a>',
    'class' => array('ajax'),
  );

  // Status
  $row['data']['status'] = empty($conf['status'][$row['data']['status']]) ?
    $row['data']['status'] :
    $conf['status'][$row['data']['status']];
  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['status']));

  // Add a link to the website
  $display = $host;

  // Link to website
  $row['data']['host'] = $display . '<a class="external-link" href="' . $row['data']['url'] . '" onclick="window.open(this.href); return false;"><img src="/images/external.png" /></a>';
  unset($row['data']['url']);

  // Link to the actual ip if different from alias
  if (($alias = urls_monitor_alias($row['data']['ip'])) && $alias != $row['data']['ip']) {
    $row['data']['ip'] = $alias . '<a class="external-link" href="javascript:alert(\'' . $row['data']['ip'] . '\'); return false;" title="' . $row['data']['ip'] . '"><img src="/images/external.png" /></a>';
  }

  // Link to see the find text for this url
  if ($row['data']['password'] == 'un-locked') {
    $row['data']['password'] .= '<a class="external-link" href="javascript:alert(\'' . $conf['httpauth'][$host] . '\'); return false;" title="' . $conf['httpauth'][$host] . '"><img src="/images/external.png" /></a>';
  }

  // Link to see the find text for this url
  if ($row['data']['text'] != URLS_MONITOR_BLANK
      && $conf['find'][$host]) {
    $row['data']['text'] .= '<a class="external-link" href="javascript:alert(\'' . $conf['find'][$host] . '\'); return false;" title="' . $conf['find'][$host] . '"><img src="/images/external.png" /></a>';
  }

  // We have to have the check column set to true or ajax won't work
  $conf['columns'] += array(
    'check' => TRUE,
  );
  $conf['columns']['check'] = TRUE;
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
 * Theme a an array as form options
 *
 * @param $options array
 *   The keys of the array are the option values, and the values of the array are the visible elements
 * @param $selected string
 *   The key of the selected option.
 *
 * @return string
 *   The html needed to add to the form.
 */
function urls_monitor_theme_options($options, $selected = '') {
  $output = '';
  foreach ($options as $key => $value) {
    $sel = $key == $selected ? ' selected="selected"' : '';
    $output .= '<option value="'. $key .'"'. $sel .'>'. $value .'</option>'."\n";
  }
  return $output;
}

/**
 * Theme a select dropdown
 *
 * @param $name string
 *   The name of the select element
 * @param $options array
 *   An array of options @see urls_monitor_theme_options()
 * @param $selected string
 * @param $label string
 * @param $description string
 *   Optional, explanatory or descriptive text
 *
 * @return html
 */
function urls_monitor_theme_select($name, $options, $selected = '', $label = '', $description = '') {
  $output = '';
  $output .= '<div class="form-item select" id="form-item-'. $name .'">'."\n";
  if ($label) {
    $output .= '<label for="'. $name .'">'. $label .'</label>'."\n";
  }
  $output .= '<select name="'. $name .'" id="'. $name .'">'."\n";
  $output .= urls_monitor_theme_options($options, $selected) ."\n";
  $output .= '</select>'."\n";
  if ($description) {
    $output .= '<div class="description">' . $description . '</div>' . "\n";
  }
  $output .= '</div> <!-- /.form-item -->'."\n";
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
  $active_page = urls_monitor_get_page();
  $config = 'config/' . $active_page;
  $q = urls_monitor_get_page();

  $options = urls_monitor_get_pages();
  $switch_to = urls_monitor_theme_select('page-menu', $options, $active_page, 'Switch to:');

  //@todo remove stylesheets that don't exist to avoid 404s
  $output = <<<EOD
<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>{$info['title']}</title>
    <meta name="description" content="Results of the domain monitor">
    <link rel="stylesheet" href="/style.css">
    <link rel="stylesheet" href="/config/default/style.css">
    <link rel="stylesheet" href="/{$config}/style.css">
    <script src="//ajax.googleapis.com/ajax/libs/jquery/1.8.0/jquery.min.js" type="text/javascript"></script>
    <script type="text/javascript" src="/jquery.tablesorter.min.js"></script>
    <script type="text/javascript" src="/urls_monitor.js"></script>
    <script type="text/javascript">var um_page='{$q}'</script>
    <script>
      $(document).ready(function(){
        $("#monitor-results").tablesorter();
      });
    </script>
  </head>
  <body>
    <h1>{$info['title']}</h1>
    <h2>{$info['subtitle']}</h2>
    {$info['body']}
    <div id="page-menu-wrapper">{$switch_to}</div>
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

function urls_monitor_fatal($message) {
  $message = '<div class="message urls_monitor_fatal">' . $message . '</div>';
  print urls_monitor_page(array('body' => $message));
  exit;
}

/** @} */ //end of group urls-monitor
