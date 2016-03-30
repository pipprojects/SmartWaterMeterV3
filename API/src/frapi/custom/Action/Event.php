<?php

/**
 * Action Event 
 * 
 * Array
 * 
 * @link http://getfrapi.com
 * @author Frapi <frapi@getfrapi.com>
 * @link /event/:id
 */
require_once(CUSTOM_LIBRARY . DIRECTORY_SEPARATOR . 'common/libraries.php');

#class Action_Event extends Frapi_Action implements Frapi_Action_Interface
class Action_Event extends Extend_Interface implements Frapi_Action_Interface
{

    /**
     * Required parameters
     * 
     * @var An array of required parameters.
     */
    protected $requiredParams = array();

    /**
     * The data container to use in toArray()
     * 
     * @var A container of data to fill and return in toArray()
     */
    public $data = array();

    public $table = "Events";

    public $name = 'events';

     public $PDefault = Array("id"=>"",
                             "timestamp"=>"",
                             "comment"=>"",
                             "device_id"=>"",
                             "device_name"=>"",
                             "value"=>"",
                             "deleted"=>0,
                             "date_created"=>"",
                             );

    /**
     * To Array
     * 
     * This method returns the value found in the database 
     * into an associative array.
     * 
     * @return array
     */
    public function toArray()
    {
        return $this->data;
    }

    /**
     * Default Call Method
     * 
     * This method is called when no specific request handler has been found
     * 
     * @return array
     */
    public function executeAction()
    {
        return $this->toArray();
    }

    /**
     * Get Request Handler
     * 
     * This method is called when a request is a GET
     * 
     * @return array
     */
    public function executeGet()
    {
        $id = $this->getParam('id', self::TYPE_STRING);

        $Params = $this->getParams();
        //print_r($Params);
        $GLOBALS['log']->lwrite(print_r($Params, true));
        if ( array_key_exists("filter", $Params) ) {
            $WhereClause = $Params["filter"];
        } else {
            $WhereClause = "";
        }
        if ( $id ) {
            $WhereID = "id = '$id'";
        } else {
            $WhereID = "";
        }
        if ( $WhereClause && $WhereID ) {
            $WhereAnd = " AND ";
        } else {
            $WhereAnd = "";
        }
        $WhereClause .= $WhereAnd . $WhereID;

        list($this->data['content'][$this->name], $this->data['sql']) = $this->CommonSelect($WhereClause);

        $TData = Array();
        $TData = $this->data['content'][$this->name];
        $CDData = $this->CommonRelated($TData, "Devices", "name", "id", "device_id", "device_name");
        $this->data['content'][$this->name] = $CDData;

        return $this->toArray();
    }

    /**
     * Post Request Handler
     * 
     * This method is called when a request is a POST
     * 
     * @return array
     */
    public function executePost()
    {
        $id = $this->getParam('id', self::TYPE_STRING);

        $this->data['content'][$this->name] = $this->CommonSelect($id, null);

        $TData = Array();
        $TData = $this->data['content'][$this->name];
        $CDData = $this->CommonRelated($TData, "Devices", "name", "id", "device_id", "device_name");
        $this->data['content'][$this->name] = $CDData;
        

        return $this->toArray();
    }

    /**
     * Put Request Handler
     * 
     * This method is called when a request is a PUT
     * 
     * @return array
     */
    public function executePut()
    {
        $id = $this->getParam('id', self::TYPE_STRING);
        $POverride = Array();
        $PDefault = Array();

        $this->data['content'] = $this->CommonChange($id, $POverride, $PDefault);

        return $this->toArray();
    }

    /**
     * Delete Request Handler
     * 
     * This method is called when a request is a DELETE
     * 
     * @return array
     */
    public function executeDelete()
    {
        return $this->toArray();
    }

    /**
     * Head Request Handler
     * 
     * This method is called when a request is a HEAD
     * 
     * @return array
     */
    public function executeHead()
    {
        return $this->toArray();
    }


}

