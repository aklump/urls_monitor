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
  $row['class'][] = urls_monitor_css_safe($row['data']['status']);

  // Add a link to the website
  $row['data']['host'] = $row['data']['host'] . ' <a class="external-link" href="' . $row['data']['url'] . '" onclick="window.open(this.href); return false;"><img src="images/external.png" /></a>';
  unset($row['data']['url']);

  $rows[] = $row;
  if (empty($header)) {
    $header = array_keys($row['data']);
  }
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
