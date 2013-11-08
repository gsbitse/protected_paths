<?php
/**
 * @file
 * Provides callbacks for configuration page.
 */

/**
 * Form for list protected paths.
 */
function protected_paths_form($form_state) {
  // Get all the paths.
  $sections = db_select('protected_paths')
    ->fields('protected_paths')
    ->execute();

  // Build the table rows.
  foreach ($sections as $section) {
    $form['protected_path:' . $section->machine_name] = array(
      '#weight' => $section->weight,
      'name' => array(
        '#markup' => $section->name,
      ),
      'weight' => array(
        '#type' => 'weight',
        '#default_value' => $section->weight,
      ),
      'machine_name' => array(
      '#type' => 'hidden',
        '#default_value' => $section->machine_name,
      ),
      'operations' => array(
        'edit' => array(
          '#type' => 'link',
          '#title' => t('Edit'),
          '#href' => PROTECTED_PATHS_ADMIN_PATH . '/' . $section->machine_name . '/edit',
          '#suffix' => ' | ',
        ),
        'delete' => array(
          '#type' => 'link',
          '#title' => t('Delete'),
          '#href' => PROTECTED_PATHS_ADMIN_PATH . '/' . $section->machine_name . '/delete',
        ),
      ),
    );
  }

  if ($sections->rowCount() == 0) {
    $form['norows'] = array(
      '#markup' => l('Add a protected path', PROTECTED_PATHS_ADMIN_PATH . '/add'),
    );
  }
  else {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
  }

  $form['#tree'] = TRUE;

  return $form;
}

/**
 * Implement the submit callback for the list form.
 */
function protected_paths_form_submit(&$form, &$form_state) {
  // Get all the machine names.
  $sections = db_select('protected_paths')
      ->fields('protected_paths', array('machine_name'))
      ->execute();

  // Save each weight
  foreach ($sections as $section) {
    if ($form_state['values']['machine_name:' . $section->machine_name]['machine_name'] == $section->machine_name) {
      $values['weight'] = $form_state['values']['machine_name:' . $section->machine_name]['weight'];
      db_update('protected_paths')
        ->fields($values)
        ->condition('machine_name', $section->machine_name)
        ->execute();
    }
  }

  drupal_set_message(t('Your configuration is saved'));
}

// Theme the table to make it draggable.
function theme_protected_paths_form($variables) {
  $form = $variables['form'];

  // Render the intro.
  $output = drupal_render($form['intro']);

  // Add the tabledrag attributes.
  drupal_add_tabledrag('protected-paths-order', 'order', 'sibling', 'protected-paths-order-weight');

  // Build the header.
  $header = array(
    t('Name'),
    t('Weight'),
    t('Operations'),
  );

  // Build the form element.
  $rows = array();
  foreach (element_children($form) as $item) {
    $element = &$form[$item];

    // Build a list of operations.
    $operations = array(drupal_render($element['operations']));

    // Add special class to be used with tabledrag.js
    if (isset($element['weight'])) {
      $element['weight']['#attributes']['class'] = array('protected-paths-order-weight');
    }

    // If we have a valid record build the row.
    if (isset($element['machine_name'])) {
      $row = array();
      $row[] = drupal_render($element['name']);
      $row[] = drupal_render($element['weight']);
      $row = array_merge($row, $operations);
      $row = array('data' => $row);
      $row['class'][] = 'draggable';
      $rows[] = $row;
    }
  }

  // Theme it as a table.
  $output .= theme_table(array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'protected-paths-order')));

  // Render the rest of the form.
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Form for add protected paths.
 */
function protected_paths_section_form($form, &$form_state, $protected_path = NULL) {
  $form['alias_list'] = array(
    '#type' => 'fieldset',
    '#title' => t('Protected Path'),
    '#description' => t('Enter the protected path and password.
     For example, "page" will protect only the url yoursite.com/page and
     "page/*" will protect every page underneath yoursite.com/page.'),
  );
  $form['alias_list']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Enter a name for the path'),
    '#required' => TRUE,
    '#default_value' => $protected_path ? $protected_path['name'] : '',
  );
  $form['alias_list']['machine_name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine name'),
    '#description' => t('Machine name of the path.'),
    '#required' => TRUE,
    '#default_value' => $protected_path ? $protected_path['machine_name'] : '',
    '#machine_name' => array(
      'exists' => 'protected_paths_load',
      'source' => array('alias_list', 'name'),
      'replace_pattern' => '[^a-z0-9-]+',
      'replace' => '-',
    ),
  );
  $form['alias_list']['paths'] = array(
    '#type' => 'textarea',
    '#title' => t('Paths'),
    '#description' => t('Enter paths to be protected. For example, "page", "page/*" etc.'),
    '#required' => TRUE,
    '#default_value' => $protected_path ? $protected_path['paths'] : '',
  );
  $form['alias_list']['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#description' => t('Enter a message to display to the user when they are presented with the password protection page.'),
    '#default_value' => $protected_path ? $protected_path['message'] : '',
  );
  $form['alias_list']['password'] = array(
    '#type' => 'password_confirm',
    '#size' => 25,
    '#required' => (empty($protected_path)) ? TRUE : FALSE,
  );
  $form['alias_list']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['alias_list']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('protected_paths_cancel'),
    '#limit_validation_errors' => array(),
  );

  return $form;
}

/**
 * protected paths form submission.
 */
function protected_paths_section_form_submit($form, &$form_state) {
  // Build the data array.
  $data = array(
    'name' => $form_state['values']['name'],
    'machine_name' => $form_state['values']['machine_name'],
    'paths' => $form_state['values']['paths'],
    'message' => $form_state['values']['message'],
  );

  // If there is a password then hash it and add it to our data array.
  if (!empty($form_state['values']['password'])) {
    require_once DRUPAL_ROOT . '/includes/password.inc';
    $data['password'] = user_hash_password(trim($form_state['values']['password']));
  }

  // Add the data to the table.
  db_merge('protected_paths')
    ->key(array('machine_name' => $data['machine_name']))
    ->fields($data)
    ->execute();

  drupal_set_message(t('%name has been successfully saved.', array('%name' => $data['name'])));
  $form_state['redirect'] = PROTECTED_PATHS_ADMIN_PATH;
}

/**
 * Cancel callback
 */
function protected_paths_cancel($form, &$form_state) {
  $form_state['redirect'] = PROTECTED_PATHS_ADMIN_PATH;
}

/**
 * Callback function for delete protected paths.
 */
function protected_paths_delete_confirm($form, &$form_state, $protected_path) {
  $form['machine_name'] = array(
    '#type' => 'hidden',
    '#value' => $protected_path['machine_name'],
  );
  $form['name'] = array(
    '#type' => 'hidden',
    '#value' => $protected_path['name'],
  );

  $message = t('Are you sure you want to delete %name?', array('%name' => $protected_path['name']));
  return confirm_form($form, $message, PROTECTED_PATHS_ADMIN_PATH, t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Implement submit callback.
 */
function protected_paths_delete_confirm_submit($form, &$form_state) {
  db_delete('protected_paths')
      ->condition('machine_name', $form_state['values']['machine_name'])
      ->execute();

  drupal_set_message(t('%name has been successfully deleted.', array('%name' => $form_state['values']['name'])));
  $form_state['redirect'] = PROTECTED_PATHS_ADMIN_PATH;
}
