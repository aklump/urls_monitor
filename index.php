<?php
/**
 * @file
 * A domain monitoring tool
 *
 * @defgroup urls-monitor URLS Monitor
 * @{
 *
 * @todo - add whois column (https://github.com/ikyon/Whois/blob/master/whois.php)?
 * @todo - refigure the ip via header?
 */
require_once('functions.inc');
global $conf;

/**
 * Configuration Checking
 */
if (!($domains = urls_monitor_urls())) {
  urls_monitor_fatal('Please add one or more domains to urls.txt');
}

$rows = array();
foreach ($domains as $domain) {
  $row = array(
    'data' => urls_monitor_check($domain, TRUE),
  );
  urls_monitor_preprocess_row($row);
  $rows[] = $row;
  if (empty($header)) {
    $header = array_keys($row['data']);
  }
}
// Translate the headers
foreach ($header as $key => $value) {
  $header[$key] = urls_monitor_alias($value);
}

/**
 * Output the View
 */
$output = urls_monitor_theme_table($rows, array(
  'id' => 'monitor-results',
  'class' => 'tablesorter'
), $header);
print urls_monitor_page(array('body' => $output));
exit();

/** @} */ //end of group urls-monitor
