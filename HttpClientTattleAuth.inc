<?php
/**
 * Auth plugin for HTTP Client. Lots of the code has been taken and
 * adapted from the OAuth php library. Which is originally licensed 
 * under the MIT license [Copyright (c) 2007 Andy Smith] and is to 
 * be found at google code: http://code.google.com/p/oauth/
 */

class HttpClientTattleAuth implements HttpClientAuthentication {
  function __construct($key, $secret) {
    $this->key = $key;
    $this->secret = $secret;
  }
  
  public function authenticate($request) {
    $request->parameters['tattle_key'] = $this->key;
    $request->parameters['tattle_nonce'] = $this->generate_nonce();
    $request->parameters['tattle_timestamp'] = time();

    $data = $request->data;
    $data || $data = '';
    $body_hash = hash_init('sha256');
    hash_update($body_hash, $data);
    $request->parameters['tattle_body_hash'] = base64_encode(hash_final($body_hash, TRUE));

    if (isset($request->parameters['tattle_signature'])) {
      unset($request->parameters['tattle_signature']);
    }
    $base_query = $this->build_http_query($request->parameters);
    $signature = base64_encode(hash_hmac('sha256', $base_query, $this->secret, TRUE));
    $request->parameters['tattle_signature'] = $signature;
  }

  private function generate_nonce() {
    $mt = microtime();
    $rand = mt_rand();

    return md5($mt . $rand); // md5s look nicer than numbers
  }
  
  private function build_http_query($params) {
    if (!$params) return '';

    // Urlencode both keys and values
    $keys = self::urlencode_rfc3986(array_keys($params));
    $values = self::urlencode_rfc3986(array_values($params));
    $params = array_combine($keys, $values);

    // Parameters are sorted by name, using lexicographical byte value ordering.
    // Ref: Spec: 9.1.1 (1)
    uksort($params, 'strcmp');

    $pairs = array();
    foreach ($params as $parameter => $value) {
      if (is_array($value)) {
        // If two or more parameters share the same name, they are sorted by their value
        // Ref: Spec: 9.1.1 (1)
        // June 12th, 2010 - changed to sort because of issue 164 by hidetaka
        sort($value, SORT_STRING);
        foreach ($value as $duplicate_value) {
          $pairs[] = $parameter . '=' . $duplicate_value;
        }
      } else {
        $pairs[] = $parameter . '=' . $value;
      }
    }
    // For each parameter, the name is separated from the corresponding value by an '=' character (ASCII code 61)
    // Each name-value pair is separated by an '&' character (ASCII code 38)
    return implode('&', $pairs);
  }

  public static function urlencode_rfc3986($input) {
    if (is_array($input)) {
      return array_map(array('HttpClientTattleAuth', 'urlencode_rfc3986'), $input);
    } else if (is_scalar($input)) {
      return str_replace(
        '+',
        ' ',
        str_replace('%7E', '~', rawurlencode($input))
      );
    } else {
      return '';
    }
  }
}