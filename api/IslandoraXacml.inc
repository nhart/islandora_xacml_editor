<?php

/**
 *  @file Subclass Xacml to facilitate communication to Islandora/Fedora.
 *
 *  Need to have Drupal bootstrapped, due to use of module_load_includes
 */
module_load_include('inc', 'islandora_xacml_api', 'Xacml');

/**
 * Subclass Xacml to facilitate communication to Islandora/Fedora.
 *
 * Need to have Drupal bootstrapped, due to use of module_load_includes
 */
class IslandoraXacml extends Xacml {

  /**
   * The idenitfy the datastream from which this policy was obtained (and to
   * which it should be written back to).
   *
   * @var $item
   * @var $pid
   * @var $dsid
   */
  protected $item, $pid, $dsid;

  /**
   * Constructor.
   *
   * @param $pid
   *   A string containing a Fedora PID, from which to attempt to load and
   *   write back to.
   * @param $dsid
   *   A string containing a Fedora DSID, from which to attempt to load and
   *   write back to.
   * @param $xacml
   *   A string containing XACML XML, or NULL to attempt to load from the given
   *   PID and DSID.
   */
  function __construct($pid, $dsid = 'POLICY', $xacml = NULL) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $item = new Fedora_Item($pid);

    if ($xacml === NULL && isset($item->datastreams[$dsid])) {
      $xacml = $item->get_datastream_dissemination($dsid);
    }

    parent::__construct($xacml);
    $this->pid = $pid;
    $this->dsid = $dsid;
    $this->item = $item;

    $this->label = 'Xacml Policy';
    if ($this->dsid == 'CHILD_SECURITY') {
      $this->label = 'Child Security Xacml Policy';
    }
  }

  /**
   * Writes our XACML stream to our PID/DSID pair...
   */
  function writeBackToFedora() {
    //Only add relationships on POLICY datastream, as only the POLICY is enforced.
    if ($this->dsid == 'POLICY' && variable_get('islandora_xacml_api_save_relationships', TRUE)) {
      $this->writeRelations();
    }

    $xml = $this->getXmlString();
    if (array_key_exists($this->dsid, $this->item->datastreams)) {
      $this->item->modify_datastream($xml, $this->dsid, $this->label, 'text/xml');
    }
    else {
      $this->item->add_datastream_from_string($xml, $this->dsid, $this->label, 'text/xml', 'X');
    }

    return TRUE;
  }

  protected function writeRelations() {
    $viewable_by_user = 'isViewableByUser';
    $viewable_by_role = 'isViewableByRole';

    // remove all old policy related rels-ext statements
    $this->item->purge_relationships($viewable_by_user, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
    $this->item->purge_relationships($viewable_by_role, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
    foreach (array_keys($this->item->datastreams) as $dsid) {
      //XXX:  These use the RELS-EXT URI...
      $this->item->purge_dsid_relationships($dsid, $viewable_by_user, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      $this->item->purge_dsid_relationships($dsid, $viewable_by_role, NULL, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
    }

    if ($this->viewingRule->isPopulated()) {
      // recompute the new values from the policy
      $view_users = $this->viewingRule->getUsers();
      $view_roles = $this->viewingRule->getRoles();
      if (count($view_users) > 0) {
        $this->item->add_relationship($viewable_by_user, $view_users, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      }
      if (count($view_roles) > 0) {
        $this->item->add_relationship($viewable_by_role, $view_roles, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
      }
    }

    if ($this->datastreamRule->isPopulated()) {
      foreach ($this->datastreamRule->getDsids() as $dsid) {
        // recompute the new values from the policy
        //XXX:  These use the RELS-EXT URI...
        $ds_users = $this->datastreamRule->getUsers();
        $ds_roles = $this->datastreamRule->getRoles();
        if (count($ds_users) > 0) {
          $this->item->add_dsid_relationship($dsid, $viewable_by_user, $ds_users, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
        }
        if (count($ds_roles) > 0) {
          $this->item->add_dsid_relationship($dsid, $viewable_by_role, $ds_roles, ISLANDORA_RELS_EXT_URI, RELS_TYPE_PLAIN_LITERAL);
        }
      }
    }
  }

}

