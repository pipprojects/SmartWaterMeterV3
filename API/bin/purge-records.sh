#!/bin/bash
#
function GetData {
  mysql -u ${DBUSER} -p${DBPASSWORD} $DATABASE < $SQLFILE > $OUTFILE
}
#
SQLFILE="/tmp/tmp-$$.tmp"
OUTFILE="/tmp/tmpout-$$.tmp"
DBUSER="root"
DBPASSWORD="password"
DATABASE="watermeter"
#
SQL="SELECT id, TableName FROM admin_Tables;"
echo $SQL > $SQLFILE
GetData
TableIDs=$(cat $OUTFILE | tail -n +2 | awk '{printf "%s\n",$1}')
TableNames=$(cat $OUTFILE | tail -n +2 | awk '{printf "%s\n",$2}')
rm -f $SQLFILE $OUTFILE
#
echo $TableIDs
echo $TableNames
#
SQL="SELECT id FROM admin_TableActionsList WHERE ActionName='Purge';"
#
SQL="INSERT INTO admin_TableActionsList (TableId,ModifiedDate, ActionId) VALUES($tableid, '$DateTime', $Actionid);"
#
for table in $TableNames
do
  SQL="DELETE FROM $table WHERE deleted = 1"
  echo $SQL >> $SQLFILE
done
#
cat $SQLFILE
#
rm -f $DQLFILE
#
