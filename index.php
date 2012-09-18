<?php
/**
 * @file
 * A domain monitoring tool
 *
 * @defgroup urls-monitor URLS Monitor
 * @{
 */
require_once('functions.inc');
global $conf;

$domains = urls_monitor_urls();

$rows = array();
foreach ($domains as $domain) {
  $row = array(
    'data' => urls_monitor_check($domain),
  );
  $row['class'][] = urls_monitor_css_safe(urls_monitor_alias($row['data']['ip']));
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
