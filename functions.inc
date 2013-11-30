<?php
use Symfony\Component\Yaml\Yaml;

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

/**
 * Return current time as date object
 *
 * @return DateTime
 *
 * @see $conf['timezone']
 */
function urls_monitor_date_now() {
  global $conf;
  return new DateTime('now', new DateTimeZone($conf['timezone']));
}

/**
 * Return the storage directory; must be writeable
 *
 * @return string
 */
function urls_monitor_get_storage_dir() {
  $storage = getcwd() . '/config/.config';
  if (!is_dir($storage)
      || !is_dir($storage . '/vars')) {
    mkdir($storage . '/vars', 0777, TRUE);
  }
  return $storage;
}

/**
 * Get a persistent variable
 *
 * @param string $var
 *
 * @return mixed
 */
function urls_monitor_get_persistent($var) {
  $value = NULL;
  $file = urls_monitor_get_storage_dir() . '/vars/' . md5($var) . '.txt';
  if (file_exists($file)
      && ($contents = file_get_contents($file))) {
    $value = trim($contents);
  }

  return $value;
}

/**
 * Set a persistent variable
 *
 * @param string $var
 * @param string $value
 */
function urls_monitor_set_persistent($var, $value) {
  $file = urls_monitor_get_storage_dir() . '/vars/' . md5($var) . '.txt';
  $fh = fopen($file, 'w');
  fwrite($fh, $value);
  fclose($fh);
}

/**
 * Get the current page
 */
function urls_monitor_get_page() {
  if (!empty($_GET['q'])) {
    urls_monitor_set_persistent('page', $_GET['q']);
    return $_GET['q'];
  }
  elseif ($page = urls_monitor_get_persistent('page')) {
    return $page;
  }
  else {
    return 'default';
  }
}

/**
 * Get the path to a config file for the current page
 *
 * If not found, the parent directory will be search recrusively until we're
 * at the top level of the configuration dirs.
 *
 * @param  string $file The filename, e.g. urls.txt or config.ini
 *
 * @return string       The full filepath to the file
 */
function urls_monitor_get_config($file) {
  $path = getcwd() . '/config/' . urls_monitor_get_page() . "/$file";
  $path = _urls_monitor_find_config($file, $path);

  return $path;
}

function _urls_monitor_find_config($file, &$path) {
  $info = pathinfo($path);
  $basename = $info['basename'];
  if ($info['basename'] === 'config') {
    return $path;
  }
  elseif ($info['basename'] === $file && is_file($path)) {
    return $path;
  }
  else {
    $path = pathinfo($info['dirname']);
    $path = $path['dirname'] . "/$file";

    return _urls_monitor_find_config($file, $path);
  }
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
    if (preg_match('/^config\/(.+?)\/urls\.txt/', $file, $matches)) {
      $pages[$matches[1]] = $matches[1];
    }
  }
  asort($pages);
  return $pages;
}

function urls_monitor_list_files($directory, $recursive) {
  $array_items = array();
  if ($handle = opendir($directory)) {
    while (false !== ($file = readdir($handle))) {
      if ($file != '.config' && $file != "." && $file != "..") {
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
    $filename = urls_monitor_get_config('urls.txt');
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
 * @param string $conf_key
 *   (Optional) Defaults to 'alias'. The $conf key to use to search for aliases
 *
 * @return string
 *   The translated string or the raw string
 */
function urls_monitor_alias($raw, $conf_key = 'alias') {
  global $conf;
  if (!empty($conf[$conf_key])
      && is_array($conf[$conf_key])) {
    // Look for exact match
    if (array_key_exists($raw, $conf[$conf_key])) {
      return $conf[$conf_key][$raw];
    }
    // Look for wildcard
    foreach (array_keys($conf[$conf_key]) as $key) {
      if (substr($raw, 0, strlen($key)) == $key) {
        return $conf[$conf_key][$key];
      }
    }
  }

  return $raw;
}

/**
 * Return an id based on an url
 *
 * @param string $url
 *
 * @return string
 */
function urls_monitor_make_id($url) {
  return preg_replace('/^https?:\/\//', '', $url);
}

/**
 * Return an status array for a domain
 *
 * @param string $id
 * @param bool $skip_lookup
 *   Set to true to skip the lookups (used when first load since ajax does the
 *   work.)
 * @param bool $user_cache
 *   (Optional) Defaults to FALSE.
 *
 * @return array
 *   - host
 *   - status (bool)
 *   - ip
 *
 * @todo figure out why the session doesn't pass in the curl calls
 */
function urls_monitor_check($url, $skip_lookup = FALSE, $use_cache = FALSE) {
  global $conf;

  if (!($parsed = urls_monitor_parse_url($url))) {
    return;
  }

  // This is the url minus the protocol
  $id = urls_monitor_make_id($url);

  $parsed += urls_monitor_get_meta($id);

  // juggle this around so the column order is based on config.ini
  $result = array_fill_keys(array_keys($conf['columns']), NULL);
  foreach ($parsed as $key => $value) {
    $result[$key] = $value;
  }

  $result += array(
    'status'        => NULL, 
    'text'          => NULL, 
    'password'      => NULL, 
    'redirects'     => NULL, 
    'redirected'    => NULL, 
    'ip'            => NULL, 
    'ttl'           => NULL,
    'ftp_status'    => NULL,  
  );

  // Detect spaces which will cause a problem
  foreach (array_keys($result) as $key) {
    if (strstr($key, ' ')) {
      urls_monitor_fatal("Row key: <strong>\"$key\"</strong> cannot contain a space; use an alias instead. Found in <em>$id</em>.");
    }
  }

  if ($use_cache
      && isset($parsed['url'])
      && ($cached = urls_monitor_get_persistent($parsed['url']))
      && ($cached = json_decode($cached))) {
    $result = (array) $cached + $result;
  }

  // Valid types are:
  // - website: all possible dynamic fields
  // - ftp; ftp connection only
  // - comment: no dynamic fields
  $skip_website = FALSE;
  switch (urls_monitor_get_property($id, 'type')) {
    case 'comment':
      $result['status'] = NULL;
      $skip_lookup = TRUE;
      break;

    case 'ftp':
      $skip_website = TRUE;
      break;
  }

  if ($skip_lookup) {
    return $result;
  }

  if (!$skip_website) {
    // From here on out we do the heavy lifting of this
    $timeout = empty($conf['timeout']) ? 10 : $conf['timeout'];

    // are we looking for content on this page?
    $find = !empty($conf['find'][$id]);

    $curl = curl_init($result['url']);
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
      if (!empty($conf['httpauth'][$id])) {
        $userpwd = $conf['httpauth'][$id];
      }
      else {
        foreach ($conf['httpauth'] as $regex => $value) {
          if (substr($value, 0, 1) === '/') {
            $regex = trim($regex, '/');
            if (preg_match('/' . $regex . '/', $id)) {
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

    // Determine if this redirects
    if ($conf['redirect_count']) {
      $result['redirects'] = $info['redirect_count'];
    }

    if ($result['redirects']) {
      $redirect = urls_monitor_parse_url($info['url']);
      if (strcasecmp(rtrim($info['url'], '/'), rtrim($result['url'], '/'))) {
        $result['redirected'] = $redirect['host'];
        if ($conf['redirect_full_path'] &&  !empty($redirect['path'])) {
          $result['redirected'] .= $redirect['path'];
        }
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
      $needles = $conf['find'][$id];

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
  }

  // Process the ftp
  $skip_ftp = empty($result['ftph']) || empty($result['ftpu']) || empty($result['ftpp']);
  if (!$skip_ftp) {
    // do the ftp test
    if (!($stream = @ftp_connect($result['ftph']))) {
      $result['ftp_status'] = 404;
    }
    elseif (!(@ftp_login($stream, $result['ftpu'], $result['ftpp']))) {
      $result['ftp_status'] = 403;
    }
    else {
      // we're in, now test the rwx perms
      $result['ftp_status'] = array();
      $now      = urls_monitor_date_now();

      // Create a temp file to put
      $send     = tempnam(sys_get_temp_dir(), $id . '.' . $now->format('U') . '.');
      $fhandle  = fopen($send, 'w');
      fwrite($fhandle, 'FTP connection test send by urls_monitor');
      fclose($fhandle);

      // Create a path destination for get
      $receive  = tempnam(sys_get_temp_dir(), $id . '.' . $now->format('U') . '.');
      unlink($receive);

      $filename = pathinfo($send);
      $filename = $filename['basename'];

      $filename = 'test';

      // Create a folder and chdir in
      if (!@ftp_mkdir($stream, $filename)) {
        if (@ftp_get($stream, $receive, $filename, FTP_ASCII)) {
          $result['ftp_status'] = 409;
        }
        $result['ftp_status'] = 405;
      }
      elseif (@ftp_chdir($stream, $filename)) {
        $result['ftp_status'][] = 'x';
        if (@ftp_put($stream, $filename, $send, FTP_ASCII)) {
          $result['ftp_status'][] = 'w';
          if (@ftp_get($stream, $receive, $filename, FTP_ASCII)) {
            $result['ftp_status'][] = 'r';
          }
          @ftp_delete($stream, $filename);
          @ftp_chdir($stream, '..');
          @ftp_rmdir($stream, $filename);
        }

        sort($result['ftp_status']);
        $result['ftp_status'] = implode($result['ftp_status']);
      }

      @ftp_close($stream);
    }
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
 *   - url
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



  // Try to get the authoritative nameserver so the TTL is not cached
  $cmd = $dig . ' +nssearch ' . $url;
  exec($cmd, $output, $return);

  $cmd = $dig . ' +noauthority +noquestion +nostats ' . $url;
  $auth_ns = FALSE;
  if (preg_match('/(?:\d{1,3}\.){3}\d{1,3}/', end($output), $found)) {
    $auth_ns = $found[0];
    $cmd = $dig . ' +noauthority +noquestion +nostats @' . $auth_ns . ' ' . $url;
  }

  // Get the TTL
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
        'ttl' => (!$auth_ns ? '~' : '') . $parsed[1],
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

  $id = urls_monitor_make_id($row['data']['url']);

  // Status alias and class
  if (!empty($row['data']['status'])) {
    $row['class'][] = urls_monitor_css_safe('status-' . $row['data']['status']);
    $row['data']['status'] = empty($conf['status'][$row['data']['status']]) ?
      $row['data']['status'] :
      $conf['status'][$row['data']['status']];
  }
  
  // FTP Status alias and class
  if (!empty($row['data']['ftp_status'])) {
    $row['class'][] = urls_monitor_css_safe('ftp-' . $row['data']['ftp_status']);
    $row['data']['ftp_status'] = empty($conf['ftp_status'][$row['data']['ftp_status']]) ?
      $row['data']['ftp_status'] :
      $conf['ftp_status'][$row['data']['ftp_status']];
  }

  $type = ($type = urls_monitor_get_property($id, 'type')) ? $type : 'website';
  $row['class'][] = urls_monitor_css_safe($type);


  foreach ($row['data'] as $key => $value) {
    if (empty($value)) {
      $row['data'][$key] = URLS_MONITOR_BLANK;
    }
  }

  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['ip']));
  
  // Do not set these values for comments
  if ($type === 'comment') {
    $row['data']['check'] = URLS_MONITOR_BLANK;
    $row['class'][] = 'comment';
  }
  else {
    // Ajax reload
    $row['data']['check'] = array(
      'data' => '<a href="#" onclick="return false;" class="ajax-check" rel="' . $row['data']['url'] . '"><img src="/images/reload.gif" alt="ajax-reload-icon" /></a>',
      'class' => array('ajax'),
    );
  }
  
  // Link to website
  $row['data']['url'] = $id;
  if ($type !== 'comment') {
    $row['data']['url'] .= '<a class="external-link" href="' . $row['data']['url'] . '" onclick="window.open(this.href); return false;"><img src="/images/external.png" /></a>';
  }
  unset($row['data']['host']);

  if (isset($row['data']['ftp_status']) && !empty($row['data']['ftpu']) && !empty($row['data']['ftpp']) && !empty($row['data']['ftph'])) {
    $link = 'ftp://' . $row['data']['ftpu'] . ':' . $row['data']['ftpp'] . '@'. $row['data']['ftph'];
    $row['data']['ftp_status'] .= '<a class="external-link" href="' . $link . '" onclick="window.open(this.href); return false;"><img src="/images/external.png" /></a>';
  }

  // Link to the actual ip if different from alias
  if (($alias = urls_monitor_alias($row['data']['ip'])) && $alias != $row['data']['ip']) {
    $row['data']['ip'] = $alias . '<a class="external-link" href="javascript:alert(\'' . $row['data']['ip'] . '\'); return false;" title="' . $row['data']['ip'] . '"><img src="/images/external.png" /></a>';
  }

  // Link to see the find text for this url
  if ($row['data']['password'] == 'un-locked') {
    $row['data']['password'] .= '<a class="external-link" href="javascript:alert(\'' . $conf['httpauth'][$id] . '\'); return false;" title="' . $conf['httpauth'][$id] . '"><img src="/images/external.png" /></a>';
  }

  // Link to see the find text for this url
  if ($row['data']['text'] != URLS_MONITOR_BLANK
      && isset($conf['find'][$id])) {
    $row['data']['text'] .= '<a class="external-link" href="javascript:alert(\'' . $conf['find'][$id] . '\'); return false;" title="' . $conf['find'][$id] . '"><img src="/images/external.png" /></a>';
  }

  // We have to have the check column set to true or ajax won't work
  $conf['columns'] += array(
    'check' => TRUE,
  );
  $conf['columns']['check'] = TRUE;
  $row['data'] = array_intersect_key($row['data'], $conf['columns']);

  urls_monitor_theme_alter(__FUNCTION__, $row);
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
  global $conf;

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

  // Add export links if enabled
  if ($conf['exports']) {
    $links = array();
    $query = '&export=1&q=' . urls_monitor_get_page();
    $links[] = array('CSV (.csv)', 'index.php?format=csv' . $query);
    $links[] = array('Excel (.xls)', 'index.php?format=xls' . $query);
    $links[] = array('Web (.html)', 'index.php?format=html' . $query);
    $links[] = array('FlatFile (.txt)', 'index.php?format=txt' . $query);
    $links[] = array('XML (.xml)', 'index.php?format=xml' . $query);
    $links[] = array('Javascript (.json)', 'index.php?format=json' . $query);
    $links[] = array('YAML (.yaml)', 'index.php?format=yaml' . $query);
    foreach ($links as $key => $link) {
      $links[$key] = '<a href="/' . $link[1] . '">' . $link[0] . '</a>' . "\n";
    }
    $links = array_reverse($links);
    $output .= '<div class="links export-links">' . implode(' ', $links) . '</div>' . "\n";
  }

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
  $info['head'] = <<<EOD
<meta charset="utf-8">
<title>{$info['title']}</title>
<meta name="description" content="Results of the domain monitor">
<link rel="stylesheet" href="/style.css">
<link rel="stylesheet" href="/themes/{$conf['theme']}/style.css">
<link rel="stylesheet" href="/{$config}/style.css">
<script src="//ajax.googleapis.com/ajax/libs/jquery/1.8.0/jquery.min.js" type="text/javascript"></script>

<script type="text/javascript" src="/scripts/canvas.js"></script>
<script type="text/javascript" src="/scripts/jquery.tablesorter.min.js"></script>
<script type="text/javascript" src="/scripts/urls_monitor.js"></script>
<script type="text/javascript">var um_page='{$q}'</script>
<script>
  $(document).ready(function(){
    $("#monitor-results").tablesorter();
  });
</script>
EOD;

  $info['body'] = <<<EOD
<h1>{$info['title']}</h1>
<h2>{$info['subtitle']}</h2>
{$info['body']}
<div id="page-menu-wrapper">{$switch_to}</div>
<div id="loading">
  <div id="loader-image"></div>
  Checking Now...
</div> 
EOD;


  urls_monitor_theme_alter(__FUNCTION__, $info);

  $output = "<!doctype html>
<html>
  <head>
  {$info['head']}
  </head>
  <body>
  {$info['body']}
  </body>
</html>";

  return $output;
}

/**
 * Call a theme alter function
 *
 * @param  string $hook e.g. 'urls_monitor_preprocess_row'
 * @param  array &$info This will be passed by recode_file
 *
 */
function urls_monitor_theme_alter($hook, &$info) {
  global $conf;
  $function = $conf['theme'] . '_'. $hook .'_alter';
  if (function_exists($function)) {
    $function($info);
  }  
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

/**
 * Returns the meta data for a domain to mix in to the dynamic data
 *
 * @param  string $domain As listed in urls.txt
 *
 * @return array         Keys are the column names
 */
function urls_monitor_get_meta($domain) {
  static $meta = array();
  if (!array_key_exists($domain, $meta)) {
    $meta[$domain] = array();
    $path = urls_monitor_get_config('meta.yaml');
    if (is_readable($path) && is_file($autoload = dirname(__FILE__) . '/vendor/autoload.php')) {
      require_once $autoload;
      $yaml = Yaml::parse(file_get_contents($path));
      foreach ($yaml as $value) {
        if (strcasecmp($value['url'], $domain) === 0) {
          ksort($value);
          $id = urls_monitor_make_id($domain);
          $meta[$id] = array(
            'url' => $domain, 
          ) + $value;
          break;
        }
      }
    }
  }

  return $meta[$domain];
}

/**
 * Return a property for a given domain
 *
 * @param  string $id       The domain id   
 * @param  string $property Something like 'type', 'find' or 'httpauth';
 *
 * @return string|NULL           
 */
function urls_monitor_get_property($id, $property) {
  static $properties = array();
  if (!array_key_exists($id, $properties)) {
    $properties[$id] = array();
    if ($meta = urls_monitor_get_meta($id)) {
      foreach ($meta as $key => $value) {
        if (substr($key, 0, 1) === '_') {
          $properties[$id][substr($key, 1)] = $value;
        }
      }
    }
  }
  
  return array_key_exists($property, $properties[$id]) ? $properties[$id][$property] : NULL;
}

/** @} */ //end of group urls-monitor
