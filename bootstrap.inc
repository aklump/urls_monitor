<?php
/**
 * @file
 * Boostrap file to load config, etc
 *
 * @ingroup monitor
 * @{
 */
global $conf;
$config_file = urls_monitor_get_config('config.ini');
if (!file_exists($config_file)) {
  urls_monitor_fatal('Please create config.ini and try again.');
}

$conf = parse_ini_file($config_file);

// Set default values of user settings, not dynamic fields
$conf += array(
  'timezone' => 'America/Los_Angeles',
  'export_filename' => '${project}_${year}${month}${day}_${hour}${second}', 
  'alias' => array(),
  'status' => array(),
  'columns' => array(),
  'find' => array(),
  'httpauth' => array(),
  'ftph' => NULL, 
  'ftpp' => NULL, 
  'ftpu' => NULL, 
  'theme' => 'default',
);
$conf['columns'] = array_filter($conf['columns']);

if (file_exists('vendor/autoload.php')) {
  require 'vendor/autoload.php';
  $conf['exports'] = class_exists('ExportData');
}

// Attach theme stuff...

// Parse the ini file from the current theme
$theme_config = "themes/" . $conf['theme'] . "/config.ini";
if (file_exists($theme_config)) {
  $conf = parse_ini_file($theme_config) + $conf;
}

// Add in the theme functions if found
$file = "themes/{$conf['theme']}/template.php";
if (file_exists($file)) {
  include_once $file;
}