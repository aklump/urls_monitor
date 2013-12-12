<?php
/**
 * @file
 * Creates/updates the page's meta.yaml file
 *
 * @ingroup monitor
 * @{
 */
use Symfony\Component\Yaml\Yaml;

require_once('functions.inc');
require_once('bootstrap.inc');
urls_monitor_set_persistent('page', (string) $argv[1]);
$path = urls_monitor_get_path_to_page('meta.yaml');

// Parse the URLS
$urls = urls_monitor_urls();
sort($urls);
$meta = array();

// Get the meta data for each url
foreach ($urls as $url) {
  $meta[] = array_filter(urls_monitor_get_meta($url));
}

// Write the meta.yaml file
if (!empty($meta)) {
  $fh = fopen($path, 'w');
  $contents = Yaml::dump($meta);
  fwrite($fh, $contents);
  fclose($fh);  
}
