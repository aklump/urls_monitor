<?php
/**
 * @file
 * A domain monitoring tool
 *
 * @defgroup urls-monitor URLS Monitor
 * @{
 */
require_once('functions.inc');

$domains = get_domains();

$rows = array();
foreach ($domains as $domain) {
  $row = array(
    'data' => ping($domain),
  );
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
$output = theme_table($rows, 'monitor-results', $header);
print page(array(
  'title' => 'Domain Monitor Results',
  'body' => $output,
));
exit();


/** @} */ //end of group urls-monitor
