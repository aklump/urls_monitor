<?php
/**
 * @file
 * Updates a meta.yaml file
 *
 * @ingroup monitor
 * @{
 */

use Symfony\Component\Yaml\Yaml;
require_once('functions.inc');
urls_monitor_set_persistent('page', (string) $argv[1]);
$path = urls_monitor_get_config('meta.yaml');

// Parse the URLS
$urls = urls_monitor_urls();
sort($urls);
$meta = array();

// Mix in current
foreach ($urls as $url) {
  $meta[] = array(
    'url' => $url, 
  ) + urls_monitor_get_meta($url);
}

// Delete if empty
if (is_file($path)) {
  unlink($path);
}

// Write the meta.yaml
if (!empty($meta)) {
  $fh = fopen($path, 'w');
  fwrite($fh, Yaml::dump($meta));
  fclose($fh);  
}
  