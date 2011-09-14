<?php

function _tattlebird_cron($output = FALSE) {
  $info = tattlebird_collect_information();
  $api = tattlebird_api();

  try {
    $response = $api->post('api/site/update-status', $info);
  } catch (HttpClientException $e) {
    $response = array('status'=>'error', 'message'=>$e->getMessage());
  }

  $out = null;
  if ($output) {
    $out = json_encode($response);
  }

  return $out;
}

function tattlebird_update_status_mappings() {
  static $status_mappings = array(
    UPDATE_NOT_SECURE    => 'not_secure',
    UPDATE_REVOKED       => 'revoked',
    UPDATE_NOT_SUPPORTED => 'not_supported',
    UPDATE_NOT_CURRENT   => 'not_current',
    UPDATE_CURRENT       => 'current',
    UPDATE_NOT_CHECKED   => 'not_checked',
    UPDATE_UNKNOWN       => 'unknown',
    UPDATE_NOT_FETCHED   => 'not_fetched',
  );
  return $status_mappings;
}

function tattlebird_requirement_status_mappings() {
  static $status_mappings = array(
    REQUIREMENT_INFO    => 'info', 
    REQUIREMENT_OK      => 'ok',
    REQUIREMENT_WARNING => 'warning',
    REQUIREMENT_ERROR   => 'error',
  );
  return $status_mappings;
}

function tattlebird_collect_information() {
  $info = array();
  $info['projects'] = tattlebird_get_project_status();
  $info['status'] = tattlebird_get_drupal_status();
  return $info;
}

function tattlebird_get_project_status() {
  $data = array();

  // Make sure that the update status is fresh.
  // If the update cron method has been run recently (or during this cron run)
  // this method just returns harmlessly.
  update_cron();
  if ($available = update_get_available(TRUE)) {
    module_load_include('inc', 'update', 'update.compare');
    $data = update_calculate_project_data($available);

    $status_mappings = tattlebird_update_status_mappings();

    // Get rid of all release information
    // and translate 
    foreach ($data as $project => &$status) {
      unset($status['releases']);
      $status['status_as_text'] = $status_mappings[$status['status']];
    }
  }
  return $data;
}

function tattlebird_get_drupal_status() {
  // Load .install files
  include_once './includes/install.inc';
  drupal_load_updates();

  // Check run-time requirements and status information.
  $requirements = module_invoke_all('requirements', 'runtime');
  usort($requirements, '_system_sort_requirements');

  $status_mappings = tattlebird_requirement_status_mappings();

  foreach ($requirements as &$status) {
    if (!isset($status['severity'])) {
      $status['severity'] = REQUIREMENT_INFO;
    }
    $status['severity_as_text'] = $status_mappings[$status['severity']];
  }

  return $requirements;
}