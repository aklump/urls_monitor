<?php
/**
 * @file
 * Ajax responses handler
 *
 * @ingroup urls_monitor
 * @{
 */
require_once('functions.inc');
require_once('bootstrap.inc');

if (!empty($_REQUEST['op']) && function_exists($_REQUEST['op'])) {
  switch ($_REQUEST['op']) {
    case 'urls_monitor_check':
      $args = array($_GET['host']);
      $return = call_user_func_array($_REQUEST['op'], $args);
      $output = json_encode($return);
      urls_monitor_set_persistent(implode(':', $args), $output);

      header('Content-Type: application/json');
      break;

    case 'urls_monitor_theme_tr':
      $row = array(
        'data' => $_POST,
      );
      urls_monitor_preprocess_row($row);
      $args = array($row, 0);
      $output = call_user_func_array($_REQUEST['op'], $args);
      header('Content-Type: text/html');
      break;
  }
  exit($output);
}
else {
  header('HTTP/1.0 404 Not Found');
  exit;
}


/** @} */ //end of group: urls_monitor
