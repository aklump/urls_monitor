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
require_once('bootstrap.inc');

/**
 * Configuration Checking
 */
if (!($domains = urls_monitor_urls())) {
  urls_monitor_fatal('Please add one or more domains to urls.txt');
}

$rows = array();
foreach ($domains as $domain) {
  $row = array(
    'data' => urls_monitor_check($domain, TRUE, isset($_GET['export']) && $_GET['export']),
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
 * Export the View
 */
if (isset($_GET['export']) && $_GET['export']) {
  $export = new ExportData();
  foreach (array_keys($rows) as $row_key) {
    foreach ($rows[$row_key]['data'] as $key => $value) {
      switch ($key) {
        case 'check':
          continue;
        default:
          $value = strip_tags($value);
          if ($value === URLS_MONITOR_BLANK) {
            $value = '';
          }
          $export->add($key, $value);
          break;
      }
    }
    $export->next();
  }

  switch ($_GET['format']) {
    case 'csv':
      $exporter = new CSVExporter($export);
      break;
    case 'xls':
      $exporter = new XLSXExporter($export);
      break;
    case 'html':
      $exporter = new HTMLExporter($export);
      break;
    case 'txt':
      $exporter = new FlatTextExporter($export);
      break;
    case 'xml':
      $exporter = new XMLExporter($export);
      break;
    case 'json':
      $exporter = new JSONExporter($export);
      break;
    case 'yaml':
      $exporter = new YAMLExporter($export);
      break;
  }
  $now = urls_monitor_date_now();
  $filename = $conf['export_filename'];
  $filename = str_replace('${project}', preg_replace('/[^a-z0-9\-]/', '-', urls_monitor_get_page()), $filename);
  $filename = str_replace('${year}'   , $now->format('Y'), $filename);
  $filename = str_replace('${month}'  , $now->format('m'), $filename);
  $filename = str_replace('${day}'    , $now->format('d'), $filename);
  $filename = str_replace('${hour}'   , $now->format('H'), $filename);
  $filename = str_replace('${second}' , $now->format('s'), $filename);
  $exporter->save($filename);
}

/**
 * Output the View
 */
else {
  $output = urls_monitor_theme_table($rows, array(
    'id' => 'monitor-results',
    'class' => 'tablesorter',
  ), $header);

  print urls_monitor_page(array('body' => $output));
  exit();
}


/** @} */ //end of group urls-monitor
