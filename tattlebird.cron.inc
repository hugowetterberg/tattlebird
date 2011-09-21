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

/**
 * Returns an array mapping update status constants
 * to human readable names.
 *
 * @return array An array with human readable names keyed by
 *   update status constants.
 */
function tattlebird_update_status_mappings() {
  static $status_mappings = array(
    UPDATE_NOT_SECURE    => 'not_secure', // 1 - emergency
    UPDATE_REVOKED       => 'revoked', // 2 - alert
    UPDATE_NOT_SUPPORTED => 'not_supported', // 3 - error
    UPDATE_NOT_CURRENT   => 'not_current', // 4 - warning
    UPDATE_CURRENT       => 'current', // 5 - ok
    UPDATE_NOT_CHECKED   => 'not_checked', // -1 - unknown
    UPDATE_UNKNOWN       => 'unknown', // -2 - unknown
    UPDATE_NOT_FETCHED   => 'not_fetched', // -3 - unknown
  );
  return $status_mappings;
}

/**
 * Returns an array mapping requirement status constants
 * to human readable names.
 *
 * @return array An array with human readable names keyed by
 *   requirement status constants.
 */
function tattlebird_requirement_status_mappings() {
  static $status_mappings = array(
    REQUIREMENT_INFO    => 'info', 
    REQUIREMENT_OK      => 'ok',
    REQUIREMENT_WARNING => 'warning',
    REQUIREMENT_ERROR   => 'error',
  );
  return $status_mappings;
}

/**
 * Returns an array mapping watchdog severity constants
 * to human readable names.
 *
 * @return array An array with human readable names keyed by
 *   watchdog severity constants.
 */
function tattlebird_watchdog_severity_mappings() {
  static $severity_mappings = array(
    WATCHDOG_EMERG    => 'emergency', // 0
    WATCHDOG_ALERT      => 'alert', // 1
    WATCHDOG_CRITICAL => 'critical', // 2
    WATCHDOG_ERROR   => 'error', // 3
    WATCHDOG_WARNING => 'warning', // 4
    WATCHDOG_NOTICE => 'notice', // 5
    WATCHDOG_INFO => 'info',  // 6
  );
  return $severity_mappings;
}

function tattlebird_collect_information() {
  $info = array();
  $info['projects'] = tattlebird_get_project_status();
  $info['status'] = tattlebird_get_drupal_status();
  $info['logs'] = tattlebird_get_logs();

  drupal_alter('tattlebird_stats', $info);

  return $info;
}

function tattlebird_get_project_status() {
  $data = array();

  if (!module_exists('update')) {
    return $data;
  }

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

function tattlebird_get_logs() {
  $log_items = array();

  if (!module_exists('dblog')) {
    $last_log_update = variable_get('tattlebird_last_log_item', 0);
    $res = db_query("SELECT w.wid, u.name, w.type, w.message, w.variables, w.severity, w.link, w.location, w.hostname
    FROM {watchdog} AS w
    INNER JOIN {users} AS u ON u.uid = w.uid
    WHERE w.severity >= %d
    AND w.wid > %d", array(
      ':severity' => WATCHDOG_CRITICAL,
      ':wid' => $last_log_update,
    ));
    $max_wid = 0;
    while ($entry = db_fetch_object($res)) {
      $entry->message = unserialize($entry->message);
      $entry->variables = unserialize($entry->variables);
      $log_items[] = $entry;
      $max_wid = $entry->wid > $max_wid ? $entry->wid : $max_wid;
    }
    variable_set('tattlebird_last_log_item', $max_wid);
  }
  return $log_items;
}
