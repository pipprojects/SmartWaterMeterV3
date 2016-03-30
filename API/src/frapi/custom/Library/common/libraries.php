<?php

class Extend_Interface extends Frapi_Action
{
//
// Check the parameter list to get the types
//
    public function CheckParamKey($Key, $value, $Params, $SQLVar, $Type, $PExist, $PDOData) {
#
        $UsePDO = $PDOData["use"];
        $PDONum = $PDOData["num"];
        $PDOPrefix = $PDOData["prefix"];
        $PDOData = Array();
        #print_r($PDOData);
#
        #if ( array_key_exists($Key, $Params) ) {
            #echo "Exists\n";
            #$Data = $this->getParam($Key, $Type);
            $Data = $value;
            $Exist = True;
            if ( $Type == self::TYPE_STRING ) {
                if ( ! $UsePDO ) {
                    $SData = $Data;
                    $PDOVar = "";
                    $DL = "'";
                } else {
                    $PDONum += 1;
                    $PDOVar = ":" . $PDOPrefix . (string)$PDONum;
                    $SData = $PDOVar;
                    $DL = "";
                }
                $PDOData = Array("num"=>$PDONum, "name"=>$PDOVar, "value"=>$Data);
            } else {
                $DL = "";
                $SData = (string)$Data;
                $PDOData = Array("num"=>$PDONum);
            }
            if ( $PExist ) {
                $CDL = ",";
            } else {
                $CDL = "";
            }
            #echo "Data $Data\n";
            $SQLData = $DL . $SData . $DL;
            $SQLReturn = $CDL . $SQLVar . "=" . $SQLData;
        #} else {
        #    $Data = "";
        #    $Exist = False;
        #    $SQLReturn = "";
        #}

        return Array($Exist, $Data, $SQLReturn, $PDOData);
    }
//
// A common function to select data from the database
//
    public function CommonSelect($WhereClause)
    {
        $Where = " WHERE ";
        //$WhereClause = "";

        //if ( $id ) {
        //    $WhereID = "id = '$id'";
        //} else {
        //    $WhereID = "";
        //}
        //if ( $WhereClause && $WhereID ) {
        //    $WhereAnd = " AND ";
        //} else {
        //    $WhereAnd = "";
        //}
        //$WhereClause .= $WhereAnd . $WhereID;

        $WhereDeleted = "deleted = '0'";
        if ( empty($WhereClause) ) {
            $WhereAnd = "";
        } else {
            $WhereAnd = " AND ";
        }
        $WhereClause .= $WhereAnd . $WhereDeleted;
        if ( empty($WhereClause) ) {
            $Where = "";
        } else {
            $Where .= $WhereClause;
        }

        $db = Frapi_Database::getInstance();
        $sqlStmt = 'SELECT * FROM ' . $this->table . $Where;
        //echo $sqlStmt;
        $stm = $db->prepare($sqlStmt);
        $stm->execute();

        #return array('devices' => $stm->fetchAll(PDO::FETCH_ASSOC));
        return Array($stm->fetchAll(PDO::FETCH_ASSOC), $sqlStmt);
    }
//
// A common function to get the data related to the main type
//
    public function CommonRelated($TData, $Table, $RelColName, $RelColID, $RelIDField, $RelNameField)
    {
        #foreach ( $this->data['content'][$this->name] as $key=>$value ) {
        foreach ( $TData as $key=>$value ) {
            $relid = $value[$RelIDField];
            $db = Frapi_Database::getInstance();
            $sqlStmt = 'SELECT ' . $RelColName . ' FROM ' . $Table . ' WHERE ' . $RelColID . " = '". $relid . "'";
            $stm = $db->prepare($sqlStmt);
            $stm->execute();
            $List = $stm->fetchAll(PDO::FETCH_ASSOC);
            if ( ! empty($List) ) {
                $Return = $List[0][$RelColName];
            } else {
                $Return = "";
            }
            #echo "Key $key\n";
            #echo "RelNameField $RelNameField\n";
            #print_r($this->data['content'][$this->name]);
            #print_r($this->data['content'][$this->name][$key]);
            #print_r($this->data['content'][$this->name][$key][$RelNameField]);
            #print_r($Return);
            #$this->data['content'][$this->name][$key][$RelNameField] = $Return;
            $TData[$key][$RelNameField] = $Return;
        }
        return $TData;
    }
//
// A common function to update or insert into the database
//
    public function CommonChange($id, $POverride, $PDefault)
    {
        if ( $id ) {
            $SQLCommand="UPDATE";
            $Where = " WHERE id = '$id'";
        } else {
            $id=$A=md5(uniqid());
            $POverride["id"] = $id;
            $SQLCommand="INSERT";
            $Where = "";
        }
        $Params = $this->getParams();

# Override input parameters
        foreach ( $POverride as $key=>$value ) {
            $Params[$key] = $value;
        }
        #print_r($Params);

        foreach ( $PDefault as $key=>$value ) {
            if ( !array_key_exists($key, $Params) ) {
                $Params[$key] = $PDefault[$key];
            }
        }

        $db = Frapi_Database::getInstance();

        $sqlStmt = "show columns from " . $this->table;
        $stm = $db->prepare($sqlStmt);
        $stm->execute();
        $Types = $stm->fetchAll(PDO::FETCH_ASSOC);
        #print_r($Types);

        $ATypes = Array();
        foreach ( $Types as $Key=>$Value ) {
            $ATypes[$Value["Field"]] = $Value;
        }
        #print_r($ATypes);

        $TypeTranslate = Array("varchar"=>self::TYPE_STRING,
            "tinyint"=>self::TYPE_INT,
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

        $sqlStmt = $SQLCommand . " " . $this->table . " SET ";
        $Exist = False;

        $PDOData = Array();
        $PDOVars = Array();
        $UsePDO = true;
        $PDONum = 0;

        foreach ( $Params as $Key=>$value ) {
            #echo "Key $Key  value $value \n";
            #echo "ATypes " . $ATypes[$Key]["Type"];
            #echo "End";
            if ( array_key_exists($Key, $ATypes) ) {
                $SType = preg_replace("/\(.*\)/", "", $ATypes[$Key]["Type"]);
                $fieldType = $TypeTranslate[$SType];
                #print $fieldType;
                $PDOData = Array("num"=>$PDONum, "use"=>$UsePDO, "prefix"=>"text");
                $PDONumP = $PDONum;
                list($Exist, $field, $SQLfield, $PDOData) = $this->CheckParamKey($Key, $value, $Params, $Key, $fieldType, $Exist, $PDOData);
                $sqlStmt .= $SQLfield;
                $PDONum = $PDOData["num"];
                if ( $PDONum != $PDONumP ) {
                    $PDOVars[$PDOData["name"]] = $PDOData["value"];
                }
            }
        }

        $sqlStmt .= $Where;
        #echo $sqlStmt . "\n";

        $stm = $db->prepare($sqlStmt);
        if ( $UsePDO ) {
            foreach ( $PDOVars as $key=>$value) {
                #echo "key $key  value  $value \n";
                $stm->bindValue($key, $value, PDO::PARAM_STR);
            }
        }
        #echo "Finsihed binParam";
        $stm->execute();
        $lastId = $db->lastInsertId();
        #$this->data['content'] = array(
        #    'count' => $stm->rowCount()
        #);
        

        return array('count' => $stm->rowCount(), $this->name=>$Params);
    }

}
