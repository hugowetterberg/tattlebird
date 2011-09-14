<?php

function tattlebird_settings_form() {
  $form = array();

  $form['tattlebird_api_server'] = array(
    '#type' => 'textfield',
    '#title' => t('Tattlebird server'),
    '#default_value' => variable_get('tattlebird_api_server', ''),
  );

  $form['tattlebird_api_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Tattlebird port'),
    '#default_value' => variable_get('tattlebird_api_port', TATTLEBIRD_DEFAULT_PORT),
  );

  $form['tattlebird_api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API key'),
    '#default_value' => variable_get('tattlebird_api_key', ''),
  );

  $form['tattlebird_api_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('API secret'),
    '#default_value' => variable_get('tattlebird_api_secret', ''),
  );

  return system_settings_form($form);
}
