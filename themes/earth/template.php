<?php
/**
 * @file
 * Functions for the theme
 */

/**
 * Alter the page info before it's output
 *
 * @param  array &$info page info
 */
function earth_urls_monitor_page_alter(&$info) {
  $theme = urls_monitor_make_url(urls_monitor_get_path_to_theme());
  $info['loading'] = '<div id="loading"><img src="' . $theme . '/images/checking.png" alt="Loading image" id="loaderImage"></div>';
  $info['head'] .= "<link href='http://fonts.googleapis.com/css?family=Eagle+Lake' rel='stylesheet' type='text/css'>\n";
}

function earth_urls_monitor_preprocess_row_alter(&$row) {
  $theme = urls_monitor_make_url(urls_monitor_get_path_to_theme());

  // Replace the row img tags with theme image files...
  foreach (array_keys($row['data']) as $key) {
    if (is_array($row['data'][$key])) {
      $row['data'][$key]['data'] = str_replace('src="/images/', "src=\"{$theme}/images/", $row['data'][$key]['data']);
      $row['data'][$key]['data'] = str_replace('/reload.gif', '/reload.png', $row['data'][$key]['data']);
    }
    else {
      $row['data'][$key] = str_replace('src="/images/', "src=\"{$theme}/images/", $row['data'][$key]);
    }
  }
}