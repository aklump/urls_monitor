<?php
/**
 * @file
 * A domain monitoring tool
 *
 * @defgroup urls-monitor URLS Monitor
 * @{
 */
require_once('functions.inc');

$domains = urls_monitor_urls();

$rows = array();
foreach ($domains as $domain) {
  $row = array(
    'data' => urls_monitor_check($domain),
  );
  $row['class'][] = urls_monitor_css_safe($row['data']['ip']);
  if (!$row['data']['status']) {
    $row['class'][] = 'offline';
    $row['data']['status'] = 'error';
  }
  else {
    $row['data']['status'] = 'ok';
  }
  $rows[] = $row;
  if (empty($header)) {
    $header = array_keys($row['data']);
  }
}

/**
 * Output the View
 */
$output = urls_monitor_theme_table($rows, array('id' => 'monitor-results', 'class' => 'tablesorter'), $header);
print urls_monitor_page(array(
  'title' => 'Domain Monitor Results',
  'body' => $output,
));
exit();


/** @} */ //end of group urls-monitor
