<?php

/**
 * Action Users 
 * 
 * Array
 * 
 * @link http://getfrapi.com
 * @author Frapi <frapi@getfrapi.com>
 * @link /user/:user_id
 */
class Action_Users extends Frapi_Action implements Frapi_Action_Interface
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
    private $data = array();

    private $table = "users";

    private function CheckParamKey( $Key, $Params, $SQLVar, $Type, $PExist) {
        if ( array_key_exists($Key, $Params) ) {
            $Data = $this->getParam($Key, $Type);
            $Exist = True;
            if ( $Type == self::TYPE_STRING ) {
                $DL = "'";
                $SData = $Data;
            } else {
                $DL = "";
                $SData = (string)$Data;
            }
            if ( $PExist ) {
                $CDL = ",";
            } else {
                $CDL = "";
            }
            $SQLData = $DL . $SData . $DL;
            $SQLReturn = $CDL . $SQLVar . "=" . $SQLData;
        } else {
            $Data = "";
            $Exist = False;
            $SQLReturn = "";
        }

        return Array($Exist, $Data, $SQLReturn);
    }
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
        // Fetch $_GET['name'] as is.
        $nameNoCast   = $this->getParam('name');
        
        // Fetch the $_GET['name'] as a string.
        $nameAsString = $this->getParam('name', self::TYPE_STRING);
        
        // Fetch the $_GET['name'] as an 
        // htmlspecialentities/htmlspecialchars string.
        $nameAsHtmlEscaped = $this->getParam('name', self::TYPE_OUTPUT);
        
        // Fetch the $_GET['name'] as an int
        $nameAsInteger = $this->getParam('user_id', self::TYPE_INT);

        #print_r($this);
        $id = $this->getParam('user_id', self::TYPE_INT);
        $db = Frapi_Database::getInstance();
        $sqlStmt = 'SELECT * FROM users WHERE user_id = ' . $id . '';
        $stm = $db->prepare($sqlStmt);
        $stm->execute();
        #$this->data = $stm->fetchAll(PDO::FETCH_ASSOC);
        $this->data['content'] = array(
            'users' => $stm->fetchAll(PDO::FETCH_ASSOC)
        );

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

        #print_r($this);
        $id = $this->getParam('user_id', self::TYPE_INT);
        $db = Frapi_Database::getInstance();
        $sqlStmt = 'SELECT * FROM users WHERE user_id = ' . $id . '';
        $stm = $db->prepare($sqlStmt);
        $stm->execute();
        #$this->data = $stm->fetchAll(PDO::FETCH_ASSOC);
        $this->data['content'] = array(
            'users' => $stm->fetchAll(PDO::FETCH_ASSOC)
        );

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
        $Params = $this->getParams();
        $id = $this->getParam('user_id', self::TYPE_INT);
        $db = Frapi_Database::getInstance();


        $sqlStmt = "show columns from " . $this->table;
        $stm = $db->prepare($sqlStmt);
        $stm->execute();
        $Types = $stm->fetchAll(PDO::FETCH_ASSOC);
        print_r($Types);

        $ATypes = Array();
        foreach ( $Types as $Key=>$Value ) {
            $ATypes[$Value["Field"]] = $Value;
        }
        print_r($ATypes);

        $TypeTranslate = Array("varchar"=>self::TYPE_STRING,
            "int"=>self::TYPE_INT,
            "mediumint"=>self::TYPE_INT,
            "smallint"=>self::TYPE_INT,
            "timyint"=>self::TYPE_INT,
            "float"=>self::TYPE_FLOAT,
            "decimal"=>self::TYPE_FLOAT,
            "datetime"=>self::TYPE_STRING,
            "date"=>self::TYPE_STRING,
            "time"=>self::TYPE_STRING,
            "timestamp"=>self::TYPE_INT,
        );

        $sqlStmt = "UPDATE " . $this->table . " SET ";
        $Exist = False;

        foreach ( $Params as $Key=>$value ) {
            echo "Key $Key  value $value \n";
            echo $ATypes[$Key]["Type"];
            $SType = preg_replace("/(.*/", "", $ATypes[$Key]["Type"]);
            $fieldType = $TypeTranslate[$SType];
            print $fieldType;
            list($Exist, $field, $SQLfield) = $this->CheckParamKey($Key, $Params, $Key, $fieldType, $Exist);
            $sqlStmt .= $SQLfield;
        }

        $sqlStmt .= " WHERE user_id=$id";
        echo $sqlStmt . "\n";

        $stm = $db->prepare($sqlStmt);
        $stm->execute();
        $this->data['content'] = array(
            'count' => $stm->rowCount()
        );

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
        // Consider a batch delete
        // Fetch the $_REQUEST['users'] which is 
        // users[]=1&users[]=2&users[]=3 as an array.
        $users = $this->getParam('users', self::TYPE_ARRAY);
        foreach ($users as $key => $user) {
            echo $user;
        }

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

