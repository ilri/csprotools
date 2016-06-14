#include <tclap/CmdLine.h>
#include <QFile>
#include <QtXml>

//Field Definition structure
struct fieldDef
{
  QString name;
  QString desc;
  QString type;
  int size;
  int decSize;
  QString rTable;
  QString rField;
  QString xmlCode;
  bool key;
  //bool join;
};
typedef fieldDef TfieldDef;

//Lookup value structure
struct lkpValue
{
  QString code;
  QString desc;
};
typedef lkpValue TlkpValue;

//Table structure
struct tableDef
{
    QString name;
    QString desc;
    QList<TfieldDef> fields;
    QList<TlkpValue> lkpValues;
    QString link;
    bool isLookUp;
    int pos;
};
typedef tableDef TtableDef;

QList<TtableDef> tables; //List of tables
QList<fieldDef> mainID; //Main CSPro ID

QString prefix;

void logout(QString message)
{
    QString temp;
    temp = message + "\n";
    printf(temp.toUtf8().data());
}

bool tableExist(QString table)
{
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].name == table)
            return true;
    }
    return false;
}

//This function sort values in lookuptable by code
bool lkpComp(TlkpValue left, TlkpValue right)
{
  return left.code < right.code;
}

//This function sort the tables by its position.
//The order is ascending so parent table are created before child tables.
//Lookup tables have a fix position of -1
bool tblComp(TtableDef left, TtableDef right)
{
    return left.pos < right.pos;
}

QString isLkpDuplicated(TtableDef table)
{
    QString res;
    int pos2;
    bool same;

    // We use this two lists to compare values after sorting them by code
    // because of ordering some values might not match.
    QList<TlkpValue> thisValues;
    QList<TlkpValue> currentValues;

    //Move the new list of values to a new list and sort it by code
    thisValues.clear();
    thisValues.append(table.lkpValues);
    qSort(thisValues.begin(),thisValues.end(),lkpComp);

    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].isLookUp)
        {
            //Move the current list of values to a new list and sort it by code
            currentValues.clear();
            currentValues.append(tables[pos].lkpValues);
            qSort(currentValues.begin(),currentValues.end(),lkpComp);

            if (currentValues.count() == thisValues.count()) //Same number of values
            {
                res = tables[pos].name;
                same = true;
                for (pos2 = 0; pos2 <= tables[pos].lkpValues.count()-1;pos2++)
                {
                    //Compares if an item in the list dont have same code or same description
                    if ((currentValues[pos2].code.simplified().toLower() != thisValues[pos2].code.simplified().toLower()) ||
                            (currentValues[pos2].desc.simplified().toLower() != thisValues[pos2].desc.simplified().toLower()))
                    {
                        res = "";
                        same = false;
                        break;
                    }
                }
                if (same)
                {
                    return res;
                }
            }
        }
    }
    return res;
}

QString getKeyField(QString table)
{
    int pos2;
    QString res;
    for (int pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].name == table)
        {
            for (pos2 = 0; pos2 <= tables[pos].fields.count()-1;pos2++)
            {
                if (tables[pos].fields[pos2].key == true)
                {
                    res = tables[pos].fields[pos2].name;
                    return res;
                }
            }
        }
    }
    return res;
}

QList<TlkpValue> getLkpValuesFromLink(QString link)
{
    QList<TlkpValue> res;
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].link == link)
        {
            res.append(tables[pos].lkpValues);
            break;
        }
    }
    return res;
}

QList<TfieldDef> getFields(QDomNode node, bool supress) //Return a list of Fields
{
    QList<TfieldDef> res;
    QDomNodeList list;
    QDomElement item;
    QDomNode VSVNode;
    //int pos;
    QString VLName;
    QString VLName2;
    QStringList VSData;

    bool hasYes;
    bool hasNo;
    QString sameTable;

    int lkp;
    QString tableLink;

    while (!node.isNull())
    {
        TfieldDef field;
        field.decSize = 0;
        field.key = false;
        //field.join = false;
        item = node.toElement();
        list = item.elementsByTagName("Label");
        field.desc = list.item(0).firstChild().nodeValue();
        list = item.elementsByTagName("Name");
        field.name = list.item(0).firstChild().nodeValue().toLower();
        field.xmlCode = field.name;
        list = item.elementsByTagName("Len");
        field.size = list.item(0).firstChild().nodeValue().toInt();
        list = item.elementsByTagName("DataType");


        if (list.count() > 0)
        {
            if (list.item(0).firstChild().nodeValue() == "Alpha")
                field.type = "Varchar";
            else
            {
                list = item.elementsByTagName("Decimal");
                if (list.count() > 0)
                {
                    field.type = "Decimal";
                    field.decSize = list.item(0).firstChild().nodeValue().toInt();
                }
                else
                    field.type = "INT";
            }
        }
        else
        {
            list = item.elementsByTagName("Decimal");
            if (list.count() > 0)
            {
                field.type = "Decimal";
                field.decSize = list.item(0).firstChild().nodeValue().toInt();
            }
            else
                field.type = "INT";
        }

        list = item.elementsByTagName("ValueSet");
        if (list.count() > 0)
        {
            TtableDef table;
            table.isLookUp = true;
            table.pos = -1;
            item = list.item(0).toElement();
            list = item.elementsByTagName("Label");
            table.desc = list.item(0).firstChild().nodeValue();
            list = item.elementsByTagName("Name");
            VLName = list.item(0).firstChild().nodeValue();

            /*pos = VLName.indexOf("_");
            if (pos >= 0)
                VLName = VLName.left(pos);*/

            VLName = VLName.replace("_","");

            VLName2 = VLName.toLower();
            VLName = "LKP" + VLName;

            VLName = VLName.toLower();
            table.name = VLName;



            TfieldDef lkpcode;
            lkpcode.desc = "Lookup field code";
            lkpcode.name = VLName2 + "_cod";
            lkpcode.type = field.type;
            lkpcode.size = field.size;
            lkpcode.decSize = 0;
            lkpcode.rField = "";
            lkpcode.rTable = "";
            lkpcode.key = true;
            //lkpcode.join = false;
            table.fields.append(lkpcode);

            TfieldDef lkpdesc;
            lkpdesc.desc = "Lookup field description";
            lkpdesc.name = VLName2 + "_des";
            lkpdesc.type = "varchar";
            lkpdesc.size = 120;
            lkpdesc.decSize = 0;
            lkpdesc.rField = "";
            lkpdesc.rTable = "";
            lkpdesc.key = false;
            //lkpdesc.join = false;
            table.fields.append(lkpdesc);

            list = item.elementsByTagName("ValueSetValues");
            if (list.count() > 0)
            {
                VSVNode = list.item(0).firstChild();
                while (!VSVNode.isNull())
                {

                    VSData = VSVNode.firstChild().nodeValue().split("|",QString::SkipEmptyParts);
                    TlkpValue VSValue;
                    VSValue.code = VSData[0].replace("'","").simplified();
                    if (VSData.count() > 1)
                        VSValue.desc = VSData[1].replace("'","").simplified();
                    else
                        VSValue.desc = "Value without description";
                    table.lkpValues.append(VSValue);


                    VSVNode = VSVNode.nextSibling();
                }
                list = item.elementsByTagName("Link");
                if (list.count() > 0)
                {
                    table.link = list.item(0).firstChild().nodeValue();                    
                }

            }
            else
            {                
                list = item.elementsByTagName("Link");
                if (list.count() > 0)
                {
                    tableLink = list.item(0).firstChild().nodeValue();
                    table.lkpValues.append(getLkpValuesFromLink(tableLink));                    
                }
            }


            if (supress == false)
            {
                sameTable = isLkpDuplicated(table);
                if (sameTable.isEmpty())
                {
                    if (tableExist(table.name) == false)
                        tables.append(table);
                    field.rTable = table.name;
                    field.rField = lkpcode.name;
                }
                else
                {
                    field.rTable = sameTable;
                    field.rField = getKeyField(sameTable);
                    qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                }
            }
            else
            {                
                if (table.lkpValues.count() == 2)
                {
                    hasNo = false;
                    hasYes = true;                    
                    for (lkp = 0; lkp <= table.lkpValues.count()-1;lkp++)
                    {
                        if ((table.lkpValues[lkp].code.toInt() == 0) && (table.lkpValues[lkp].desc.trimmed().toLower() == "no"))
                            hasNo = true;
                        if ((table.lkpValues[lkp].code.toInt() == 1) && (table.lkpValues[lkp].desc.trimmed().toLower() == "yes"))
                            hasYes = true;
                    }                    
                    if (hasYes && hasNo)
                    {                        
                        field.rTable = "";
                        field.rField = "";
                    }
                    else
                    {
                        sameTable = isLkpDuplicated(table);
                        if (sameTable.isEmpty())
                        {
                            if (tableExist(table.name) == false)
                                tables.append(table);
                            field.rTable = table.name;
                            field.rField = lkpcode.name;
                        }
                        else
                        {
                            field.rTable = sameTable;
                            field.rField = getKeyField(sameTable);
                            qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                        }
                    }
                }
                else
                {
                    sameTable = isLkpDuplicated(table);
                    if (sameTable.isEmpty())
                    {
                        if (tableExist(table.name) == false)
                            tables.append(table);
                        field.rTable = table.name;
                        field.rField = lkpcode.name;
                    }
                    else
                    {
                        field.rTable = sameTable;
                        field.rField = getKeyField(sameTable);
                        qDebug() << table.name + " = " + sameTable + ". " + sameTable + " used instead";
                    }
                }
            }
        }
        else
        {
            field.rField = "";
            field.rTable = "";
        }

        res.append(field);
        node = node.nextSibling();
    }

    return res;
}

/*bool relTableJoin(QString rtable, QString rfield)
{

    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].name == rtable)
        {
            for (int pos2 = 0; pos2 <= tables[pos].fields.count()-1;pos2++)
            {
                if (tables[pos].fields[pos2].name == rfield)
                    return tables[pos].fields[pos2].join;
            }
        }
    }

    return false;
}*/

//This function returns wether or not a related table is a lookup table
bool isRelatedTableLookUp(QString relatedTable)
{
    int pos;
    for (pos = 0; pos <= tables.count()-1; pos++)
    {
        if (tables[pos].name.toLower() == relatedTable.toLower())
            return tables[pos].isLookUp;
    }
    return false;
}

//void genSQL(QString ddlFile,QString insFile, QString metaFile, QString maintable, QString UUIDFile, QString xmlFile, QString XMLCreate, QString XMLInsert)
//{
//    QStringList fields;
//    QStringList indexes;
//    QStringList keys;
//    QStringList rels;
//    int clm;
//    QString sql;
//    QString keysql;
//    QString insertSQL;
//    QString field;
//    int idx;
//    idx = 0;

//    QString index;
//    QString constraint;


//    QDomDocument outputdoc;
//    outputdoc = QDomDocument("ODKImportFile");
//    QDomElement root;
//    root = outputdoc.createElement("ODKImportXML");
//    root.setAttribute("version", "1.0");
//    outputdoc.appendChild(root);

//    QDomElement eMaintable;
//    eMaintable = outputdoc.createElement("table");


//    //This is the XML representation of the schema
//    QDomDocument XMLSchemaStructure;
//    XMLSchemaStructure = QDomDocument("XMLSchemaStructure");
//    QDomElement XMLRoot;
//    XMLRoot = XMLSchemaStructure.createElement("XMLSchemaStructure");
//    XMLRoot.setAttribute("version", "1.0");
//    XMLSchemaStructure.appendChild(XMLRoot);

//    QDomElement XMLLKPTables;
//    XMLLKPTables = XMLSchemaStructure.createElement("lkptables");
//    XMLRoot.appendChild(XMLLKPTables);

//    QDomElement XMLTables;
//    XMLTables = XMLSchemaStructure.createElement("tables");
//    XMLRoot.appendChild(XMLTables);

//    QDomElement eMTableCrt;
//    eMTableCrt = XMLSchemaStructure.createElement("table");
//    XMLTables.appendChild(eMTableCrt);


//    //This is the XML representation lookup values
//    QDomDocument insertValuesXML;
//    insertValuesXML = QDomDocument("insertValuesXML");
//    QDomElement XMLInsertRoot;
//    XMLInsertRoot = insertValuesXML.createElement("insertValuesXML");
//    XMLInsertRoot.setAttribute("version", "1.0");
//    insertValuesXML.appendChild(XMLInsertRoot);



//    QFile sqlCreateFile(ddlFile);
//    if (!sqlCreateFile.open(QIODevice::WriteOnly | QIODevice::Text))
//             return;
//    QTextStream sqlCreateStrm(&sqlCreateFile);
//    sqlCreateStrm.setCodec("UTF-8");

//    QFile sqlInsertFile(insFile);
//    if (!sqlInsertFile.open(QIODevice::WriteOnly | QIODevice::Text))
//             return;
//    QTextStream sqlInsertStrm(&sqlInsertFile);
//    sqlInsertStrm.setCodec("UTF-8");

//    QFile sqlUpdateFile(metaFile);
//    if (!sqlUpdateFile.open(QIODevice::WriteOnly | QIODevice::Text))
//             return;
//    QTextStream sqlUpdateStrm(&sqlUpdateFile);
//    sqlUpdateStrm.setCodec("UTF-8");

//    QFile UUIDTriggerFile(UUIDFile);
//    if (!UUIDTriggerFile.open(QIODevice::WriteOnly | QIODevice::Text))
//             return;
//    QTextStream UUIDStrm(&UUIDTriggerFile);
//    UUIDStrm.setCodec("UTF-8");


//    QList<TfieldDef> joinrels;

//    QDateTime date;
//    date = QDateTime::currentDateTime();

//    sqlCreateStrm << "-- Code generated by CSProToMySQL" << "\n";
//    sqlCreateStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
//    sqlCreateStrm << "-- by: CSProToMySQL 1.1" << "\n";
//    sqlCreateStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

//    sqlInsertStrm << "-- Code generated by CSProToMySQL" << "\n";
//    sqlInsertStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
//    sqlInsertStrm << "-- by: CSProToMySQL 1.1" << "\n";
//    sqlInsertStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

//    UUIDStrm << "-- Code generated by CSProToMySQL 1.1" << "\n";
//    UUIDStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
//    UUIDStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

//    qSort(tables.begin(),tables.end(),tblComp);

//    for (int pos = 0; pos <= tables.count()-1;pos++)
//    {
//        QDomElement atable;
//        QDomElement aCrtTable;
//        //Add the tables to the XML manifest
//        if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
//        {
//            eMaintable.setAttribute("xmlcode","Main");
//            eMaintable.setAttribute("parent","NULL");
//            eMaintable.setAttribute("mysqlcode",prefix + tables[pos].name);
//            root.appendChild(eMaintable);

//            eMTableCrt.setAttribute("name",prefix + tables[pos].name);

//        }
//        else
//        {
//            if (tables[pos].isLookUp == false)
//            {
//                atable = outputdoc.createElement("table");
//                atable.setAttribute("xmlcode",tables[pos].name);
//                atable.setAttribute("parent",prefix + maintable.simplified().toLower());
//                atable.setAttribute("mysqlcode",prefix + tables[pos].name);
//                eMaintable.appendChild(atable);

//                aCrtTable = XMLSchemaStructure.createElement("table");
//                aCrtTable.setAttribute("name",prefix + tables[pos].name);
//                eMTableCrt.appendChild(aCrtTable);
//            }
//            else
//            {
//                aCrtTable = XMLSchemaStructure.createElement("table");
//                aCrtTable.setAttribute("name",prefix + tables[pos].name);
//                XMLLKPTables.appendChild(aCrtTable);
//            }
//        }


//        sqlUpdateStrm << "UPDATE dict_tblinfo SET tbl_des = '" + tables[pos].desc + "' WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "';\n";

//        fields.clear();
//        indexes.clear();
//        keys.clear();
//        rels.clear();
//        sql = "";
//        keysql = "";

//        joinrels.clear();

//        fields << "CREATE TABLE IF NOT EXISTS " + prefix + tables[pos].name.toLower() + "(" << "\n";
//        keys << "PRIMARY KEY (";
//        for (clm = 0; clm <= tables[pos].fields.count()-1; clm++)
//        {
//            //Add the fields to the XML manifest
//            if (tables[pos].isLookUp == false)
//            {
//                QDomElement afield;
//                afield = outputdoc.createElement("field");
//                afield.setAttribute("xmlcode",tables[pos].fields[clm].xmlCode);
//                afield.setAttribute("mysqlcode",tables[pos].fields[clm].name);
//                if (tables[pos].fields[clm].key == true)
//                {
//                    afield.setAttribute("key","true");
//                    if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
//                        afield.setAttribute("reference","false");
//                    else
//                    {
//                        if (tables[pos].fields[clm].rTable != "")
//                            afield.setAttribute("reference","true");
//                        else
//                            afield.setAttribute("reference","false");
//                    }
//                }
//                else
//                {
//                    afield.setAttribute("key","false");
//                    afield.setAttribute("reference","false");
//                }

//                if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
//                    eMaintable.appendChild(afield);
//                else
//                    atable.appendChild(afield);

//                QDomElement aCRTField;
//                aCRTField = XMLSchemaStructure.createElement("field");
//                aCRTField.setAttribute("decsize",tables[pos].fields[clm].decSize);
//                if (tables[pos].fields[clm].key == true)
//                    aCRTField.setAttribute("key","true");
//                else
//                    aCRTField.setAttribute("key","false");
//                aCRTField.setAttribute("type",tables[pos].fields[clm].type.toLower());
//                aCRTField.setAttribute("name",tables[pos].fields[clm].name.toLower());
//                aCRTField.setAttribute("size",tables[pos].fields[clm].size);
//                if (tables[pos].fields[clm].rTable != "")
//                {
//                    aCRTField.setAttribute("rtable",prefix + tables[pos].fields[clm].rTable.toLower());
//                    aCRTField.setAttribute("rfield",tables[pos].fields[clm].rField.toLower());
//                    if (isRelatedTableLookUp(tables[pos].fields[clm].rTable))
//                        aCRTField.setAttribute("rlookup","true");
//                }
//                if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
//                    eMTableCrt.appendChild(aCRTField);
//                else
//                    aCrtTable.appendChild(aCRTField);


//            }
//            else
//            {
//                QDomElement aCRTField;
//                aCRTField = XMLSchemaStructure.createElement("field");
//                aCRTField.setAttribute("decsize",tables[pos].fields[clm].decSize);
//                if (tables[pos].fields[clm].key == true)
//                    aCRTField.setAttribute("key","true");
//                else
//                    aCRTField.setAttribute("key","false");
//                aCRTField.setAttribute("type",tables[pos].fields[clm].type.toLower());
//                aCRTField.setAttribute("name",tables[pos].fields[clm].name);
//                aCRTField.setAttribute("size",tables[pos].fields[clm].size);
//                aCrtTable.appendChild(aCRTField);
//            }

//            sqlUpdateStrm << "UPDATE dict_clminfo SET clm_des = '" + tables[pos].fields[clm].desc + "' WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "' AND clm_cod = '" + tables[pos].fields[clm].name + "';\n";

//            if (tables[pos].fields[clm].type != "Decimal")
//                field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type.toLower() + "(" + QString::number(tables[pos].fields[clm].size) + ")";
//            else
//                field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type.toLower() + "(" + QString::number(tables[pos].fields[clm].size) + "," + QString::number(tables[pos].fields[clm].decSize) + ")";
//            if (tables[pos].fields[clm].key == true)
//                field = field + " NOT NULL , ";
//            else
//                field = field + " , ";
//            fields << field << "\n";

//            if (tables[pos].fields[clm].key == true)
//                keys << tables[pos].fields[clm].name.toLower() + " , ";
//            if (!tables[pos].fields[clm].rTable.isEmpty())
//            {

//                //if (relTableJoin(tables[pos].name,tables[pos].fields[clm].name) == false)
//                //{

//                    idx++;

//                    index = "INDEX fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower() ;
//                    indexes << index.left(64) + " (" + tables[pos].fields[clm].name.toLower() + ") , " << "\n";

//                    constraint = "CONSTRAINT fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower();
//                    rels << constraint.left(64) << "\n";
//                    rels << "FOREIGN KEY (" + tables[pos].fields[clm].name.toLower() + ")" << "\n";
//                    rels << "REFERENCES " + prefix + tables[pos].fields[clm].rTable.toLower() + " (" + tables[pos].fields[clm].rField.toLower() + ")" << "\n";
//                    if (tables[pos].fields[clm].rTable.toLower() == maintable.toLower())
//                        rels << "ON DELETE CASCADE " << "\n";
//                    else
//                        rels << "ON DELETE RESTRICT " << "\n";
//                    rels << "ON UPDATE NO ACTION," << "\n";
//                //}
//                //else
//                //{
//                //    joinrels.append(tables[pos].fields[clm]); //If its a join relationship we need to process it later
//                //}

//            }

//        }
//        //Process join relationships if there is any
//        if (joinrels.count() > 0)
//        {
//            idx++;
//            index = "INDEX fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower();
//            index = index.left(64) + " (";
//            constraint = "CONSTRAINT fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower();
//            rels << constraint.left(64) << "\n";

//            QString foreignKey;
//            QString references;

//            foreignKey = "FOREIGN KEY (";

//            // TODO: We asume only the multiple references only goes to one unique table
//            // This might be OK since in CSPro we are linking all child tables to the main table

//            references = "REFERENCES " + prefix + joinrels[0].rTable.toLower() + " (";

//            for (clm = 0; clm <= joinrels.count()-1; clm++)
//            {
//                index = index + joinrels[clm].name.toLower() + ",";
//                foreignKey = foreignKey + joinrels[clm].name.toLower() + ",";
//                references = references + joinrels[clm].name.toLower() + ",";
//            }

//            //Append the inddex
//            index = index.left(index.length()-1) + ") , ";
//            indexes << index << "\n";

//            //Append the foreign Key
//            foreignKey = foreignKey.left(foreignKey.length()-1) + ")";
//            rels << foreignKey << "\n";

//            //Append the references
//            references = references.left(references.length()-1) + ")";
//            rels << references << "\n";
//            rels << "ON DELETE CASCADE " << "\n";
//            rels << "ON UPDATE NO ACTION," << "\n";

//        }


//        for (clm = 0; clm <= fields.count() -1;clm++)
//        {
//            sql = sql + fields[clm];
//        }
//        for (clm = 0; clm <= keys.count() -1;clm++)
//        {
//            keysql = keysql + keys[clm];
//        }
//        clm = keysql.lastIndexOf(",");
//        keysql = keysql.left(clm) + ") , \n";

//        sql = sql + keysql;

//        for (clm = 0; clm <= indexes.count() -1;clm++)
//        {
//            sql = sql + indexes[clm];
//        }
//        for (clm = 0; clm <= rels.count() -1;clm++)
//        {
//            sql = sql + rels[clm];
//        }
//        clm = sql.lastIndexOf(",");
//        sql = sql.left(clm);
//        sql = sql + ")" + "\n ENGINE = InnoDB CHARSET=utf8; \n\n";

//        //Append UUIDs triggers to the the
//        UUIDStrm << "CREATE TRIGGER uudi_" + prefix + tables[pos].name.toLower() + " BEFORE INSERT ON " + prefix + tables[pos].name.toLower() + " FOR EACH ROW SET new.rowuuid = uuid();\n\n";

//        sqlCreateStrm << sql;

//        if (tables[pos].lkpValues.count() > 0)
//        {
//            for (clm = 0; clm <= tables[pos].lkpValues.count()-1;clm++)
//            {
//                insertSQL = "INSERT INTO " + prefix + tables[pos].name.toLower() + " (";
//                for (int pos2 = 0; pos2 <= tables[pos].fields.count()-2;pos2++)
//                {
//                    insertSQL = insertSQL + tables[pos].fields[pos2].name + ",";
//                }
//                insertSQL = insertSQL.left(insertSQL.length()-1) + ")  VALUES ('";

//                insertSQL = insertSQL + tables[pos].lkpValues[clm].code + "','";
//                insertSQL = insertSQL + tables[pos].lkpValues[clm].desc + "');";
//                sqlInsertStrm << insertSQL << "\n";
//            }
//        }
//    }

//    //Create the manifest file. If exist it gets overwriten
//    if (QFile::exists(xmlFile))
//        QFile::remove(xmlFile);
//    QFile file(xmlFile);
//    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
//    {
//        QTextStream out(&file);
//        out.setCodec("UTF-8");
//        outputdoc.save(out,1,QDomNode::EncodingFromTextStream);
//        file.close();
//    }
//    else
//        logout("Error: Cannot create xml manifest file");

//    //Create the XMLCreare file. If exist it get overwriten
//    if (QFile::exists(XMLCreate))
//        QFile::remove(XMLCreate);
//    QFile XMLCreateFile(XMLCreate);
//    if (XMLCreateFile.open(QIODevice::WriteOnly | QIODevice::Text))
//    {
//        QTextStream outXMLCreate(&XMLCreateFile);
//        outXMLCreate.setCodec("UTF-8");
//        XMLSchemaStructure.save(outXMLCreate,1,QDomNode::EncodingFromTextStream);
//        XMLCreateFile.close();
//    }
//    else
//        logout("Error: Cannot create xml create file");

//}

//This function retrieves a coma separated string of the fields
// that are related to a table
QString getForeignColumns(TtableDef table, QString relatedTable)
{
    QString res;
    int pos;
    for (pos = 0; pos <= table.fields.count()-1; pos++)
    {
        if (table.fields[pos].rTable == relatedTable)
        {
            res = res + table.fields[pos].name.toLower() + ",";
        }
    }
    res = res.left(res.length()-1);
    return res;
}

//This function retrieves a coma separated string of the related fields
// that are related to a table
QString getReferencedColumns(TtableDef table, QString relatedTable)
{
    QString res;
    int pos;
    for (pos = 0; pos <= table.fields.count()-1; pos++)
    {
        if (table.fields[pos].rTable == relatedTable)
        {
            res = res + table.fields[pos].rField.toLower() + ",";
        }
    }
    res = res.left(res.length()-1);
    return res;
}

void genSQL(QString maintable, QString ddlFile,QString insFile, QString metaFile, QString xmlFile, QString UUIDFile, QString XMLCreate, QString insertXML)
{
    QStringList fields;
    QStringList indexes;
    QStringList keys;
    QStringList rels;
    int clm;
    QString sql;
    QString keysql;
    QString insertSQL;
    QString field;
    int idx;
    idx = 0;

    QDomDocument outputdoc;
    outputdoc = QDomDocument("ODKImportFile");
    QDomElement root;
    root = outputdoc.createElement("ODKImportXML");
    root.setAttribute("version", "1.0");
    outputdoc.appendChild(root);

    QDomElement eMaintable;
    eMaintable = outputdoc.createElement("table");

    //This is the XML representation of the schema
    QDomDocument XMLSchemaStructure;
    XMLSchemaStructure = QDomDocument("XMLSchemaStructure");
    QDomElement XMLRoot;
    XMLRoot = XMLSchemaStructure.createElement("XMLSchemaStructure");
    XMLRoot.setAttribute("version", "1.0");
    XMLSchemaStructure.appendChild(XMLRoot);

    QDomElement XMLLKPTables;
    XMLLKPTables = XMLSchemaStructure.createElement("lkptables");
    XMLRoot.appendChild(XMLLKPTables);

    QDomElement XMLTables;
    XMLTables = XMLSchemaStructure.createElement("tables");
    XMLRoot.appendChild(XMLTables);

    QDomElement eMTableCrt;
    eMTableCrt = XMLSchemaStructure.createElement("table");
    XMLTables.appendChild(eMTableCrt);

    //This is the XML representation lookup values
    QDomDocument insertValuesXML;
    insertValuesXML = QDomDocument("insertValuesXML");
    QDomElement XMLInsertRoot;
    XMLInsertRoot = insertValuesXML.createElement("insertValuesXML");
    XMLInsertRoot.setAttribute("version", "1.0");
    insertValuesXML.appendChild(XMLInsertRoot);

    QDomElement fieldNode;

    QString index;
    QString constraint;

    //Create all the files and streams
    QFile sqlCreateFile(ddlFile);
    if (!sqlCreateFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream sqlCreateStrm(&sqlCreateFile);

    QFile sqlInsertFile(insFile);
    if (!sqlInsertFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream sqlInsertStrm(&sqlInsertFile);
    sqlInsertStrm.setCodec("UTF-8");



    QFile UUIDTriggersFile(UUIDFile);
    if (!UUIDTriggersFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;
    QTextStream UUIDStrm(&UUIDTriggersFile);
    UUIDStrm.setCodec("UTF-8");

    QFile sqlUpdateFile(metaFile);
    if (!sqlUpdateFile.open(QIODevice::WriteOnly | QIODevice::Text))
             return;

    QTextStream sqlUpdateStrm(&sqlUpdateFile);
    sqlUpdateStrm.setCodec("UTF-8");

    //Start creating the header or each file.
    QDateTime date;
    date = QDateTime::currentDateTime();

    QStringList rTables;

    sqlCreateStrm << "-- Code generated by CSProToMySQL" << "\n";
    sqlCreateStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    sqlCreateStrm << "-- by: CSProToMySQL Version 2.0" << "\n";
    sqlCreateStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

    sqlInsertStrm << "-- Code generated by CSProToMySQL" << "\n";
    sqlInsertStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    sqlInsertStrm << "-- by: CSProToMySQL Version 2.0" << "\n";
    sqlInsertStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

    UUIDStrm << "-- Code generated by CSProToMySQL" << "\n";
    UUIDStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    UUIDStrm << "-- by: CSProToMySQL Version 2.0" << "\n";
    UUIDStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";

    sqlUpdateStrm << "-- Code generated by CSProToMySQL" << "\n";
    sqlUpdateStrm << "-- Created: " + date.toString("ddd MMMM d yyyy h:m:s ap")  << "\n";
    sqlUpdateStrm << "-- by: CSProToMySQL Version 2.0" << "\n";
    sqlUpdateStrm << "-- WARNING! All changes made in this file might be lost when running CSProToMySQL again" << "\n\n";



    qSort(tables.begin(),tables.end(),tblComp);

    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        //We start by creating the tables in both the manifest and the XML create

        QDomElement atable;
        QDomElement aCrtTable;
        //Add the tables to the XML manifest
        if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
        {
            eMaintable.setAttribute("xmlcode","main");
            eMaintable.setAttribute("parent","NULL");
            eMaintable.setAttribute("mysqlcode",prefix + tables[pos].name);
            root.appendChild(eMaintable);

            eMTableCrt.setAttribute("name",prefix + tables[pos].name);

        }
        else
        {
            if (tables[pos].isLookUp == false)
            {
                atable = outputdoc.createElement("table");
                atable.setAttribute("xmlcode",tables[pos].name);
                atable.setAttribute("parent",prefix + maintable.simplified().toLower());
                atable.setAttribute("mysqlcode",prefix + tables[pos].name);
                eMaintable.appendChild(atable);

                aCrtTable = XMLSchemaStructure.createElement("table");
                aCrtTable.setAttribute("name",prefix + tables[pos].name);
                eMTableCrt.appendChild(aCrtTable);
            }
            else
            {
                aCrtTable = XMLSchemaStructure.createElement("table");
                aCrtTable.setAttribute("name",prefix + tables[pos].name);
                XMLLKPTables.appendChild(aCrtTable);

                //Add the table to the INSERT XML
                QDomElement lkptable = insertValuesXML.createElement("table");
                lkptable.setAttribute("name",prefix + tables[pos].name.toLower());
                lkptable.setAttribute("clmcode",tables[pos].fields[0].name);
                lkptable.setAttribute("clmdesc",tables[pos].fields[1].name);

                for (int nlkp = 0; nlkp < tables[pos].lkpValues.count();nlkp++)
                {
                    QDomElement aLKPValue = insertValuesXML.createElement("value");
                    aLKPValue.setAttribute("code",tables[pos].lkpValues[nlkp].code);
                    aLKPValue.setAttribute("description",tables[pos].lkpValues[nlkp].desc);
                    lkptable.appendChild(aLKPValue);
                }
                XMLInsertRoot.appendChild(lkptable);

            }
        }

        //Update the dictionary tables to set the table description
        sqlUpdateStrm << "UPDATE dict_tblinfo SET tbl_des = \"" + tables[pos].desc + "\" WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "';\n";

        //Clear the controlling lists
        fields.clear();
        indexes.clear();
        keys.clear();
        rels.clear();
        sql = "";
        keysql = "";

        //Start of a create script for each table
        fields << "CREATE TABLE IF NOT EXISTS " + prefix + tables[pos].name.toLower() + "(" << "\n";

        keys << "PRIMARY KEY (";
        for (clm = 0; clm <= tables[pos].fields.count()-1; clm++)
        {

            if (tables[pos].isLookUp == false)
            {
                QDomElement afield;
                afield = outputdoc.createElement("field");
                afield.setAttribute("xmlcode",tables[pos].fields[clm].xmlCode);
                afield.setAttribute("mysqlcode",tables[pos].fields[clm].name);
                if (tables[pos].fields[clm].key == true)
                {
                    afield.setAttribute("key","true");
                    if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
                        afield.setAttribute("reference","false");
                    else
                    {
                        if (tables[pos].fields[clm].rTable != "")
                            afield.setAttribute("reference","true");
                        else
                            afield.setAttribute("reference","false");
                    }
                }
                else
                {
                    afield.setAttribute("key","false");
                    afield.setAttribute("reference","false");
                }

                if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
                    eMaintable.appendChild(afield);
                else
                    atable.appendChild(afield);

                QDomElement aCRTField;
                aCRTField = XMLSchemaStructure.createElement("field");
                aCRTField.setAttribute("decsize",tables[pos].fields[clm].decSize);
                if (tables[pos].fields[clm].key == true)
                    aCRTField.setAttribute("key","true");
                else
                    aCRTField.setAttribute("key","false");
                aCRTField.setAttribute("type",tables[pos].fields[clm].type.toLower());
                aCRTField.setAttribute("name",tables[pos].fields[clm].name.toLower());
                aCRTField.setAttribute("size",tables[pos].fields[clm].size);
                if (tables[pos].fields[clm].rTable != "")
                {
                    aCRTField.setAttribute("rtable",prefix + tables[pos].fields[clm].rTable.toLower());
                    aCRTField.setAttribute("rfield",tables[pos].fields[clm].rField.toLower());
                    if (isRelatedTableLookUp(tables[pos].fields[clm].rTable))
                        aCRTField.setAttribute("rlookup","true");
                }
                if (tables[pos].name.simplified().toLower() == maintable.simplified().toLower())
                    eMTableCrt.appendChild(aCRTField);
                else
                    aCrtTable.appendChild(aCRTField);


            }
            else
            {
                QDomElement aCRTField;
                aCRTField = XMLSchemaStructure.createElement("field");
                aCRTField.setAttribute("decsize",tables[pos].fields[clm].decSize);
                if (tables[pos].fields[clm].key == true)
                    aCRTField.setAttribute("key","true");
                else
                    aCRTField.setAttribute("key","false");
                aCRTField.setAttribute("type",tables[pos].fields[clm].type.toLower());
                aCRTField.setAttribute("name",tables[pos].fields[clm].name);
                aCRTField.setAttribute("size",tables[pos].fields[clm].size);
                aCrtTable.appendChild(aCRTField);
            }


            //Update the dictionary tables to the set column description
            sqlUpdateStrm << "UPDATE dict_clminfo SET clm_des = \"" + tables[pos].fields[clm].desc + "\" WHERE tbl_cod = '" + prefix + tables[pos].name.toLower() + "' AND clm_cod = '" + tables[pos].fields[clm].name + "';\n";

            //Work out the mySQL column types
            field = "";
            if ((tables[pos].fields[clm].type.toLower() == "varchar") || (tables[pos].fields[clm].type.toLower() == "int"))
                field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type.toLower() + "(" + QString::number(tables[pos].fields[clm].size) + ")";
            else
                if (tables[pos].fields[clm].type.toLower() == "decimal")
                    field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type.toLower() + "(" + QString::number(tables[pos].fields[clm].size) + "," + QString::number(tables[pos].fields[clm].decSize) + ")";
                else
                    field = tables[pos].fields[clm].name.toLower() + " " + tables[pos].fields[clm].type.toLower();

            if (tables[pos].fields[clm].key == true)
                field = field + " NOT NULL , ";
            else
                field = field + " , ";

            fields << field << "\n";

            if (tables[pos].fields[clm].key == true)
                keys << tables[pos].fields[clm].name.toLower() + " , ";

            //Here we create the indexes and constraints for lookup tables using RESTRICT

            if (!tables[pos].fields[clm].rTable.isEmpty())
            {
                if (isRelatedTableLookUp(tables[pos].fields[clm].rTable))
                {
                    idx++;
                    index = "INDEX fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower() ;
                    indexes << index.left(64) + " (" + tables[pos].fields[clm].name.toLower() + ") , " << "\n";

                    constraint = "CONSTRAINT fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + tables[pos].fields[clm].rTable.toLower();
                    rels << constraint.left(64) << "\n";
                    rels << "FOREIGN KEY (" + tables[pos].fields[clm].name.toLower() + ")" << "\n";
                    rels << "REFERENCES " + prefix + tables[pos].fields[clm].rTable.toLower() + " (" + tables[pos].fields[clm].rField.toLower() + ")" << "\n";

                    rels << "ON DELETE RESTRICT " << "\n";
                    rels << "ON UPDATE NO ACTION," << "\n";
                }
            }
        }

        rTables.clear();
        //Extract all related tables into rTables that are not lookups
        for (clm = 0; clm <= tables[pos].fields.count()-1; clm++)
        {
            if (!tables[pos].fields[clm].rTable.isEmpty())
            {
                if (!isRelatedTableLookUp(tables[pos].fields[clm].rTable))
                    if (rTables.indexOf(tables[pos].fields[clm].rTable) < 0)
                        rTables.append(tables[pos].fields[clm].rTable);
            }
        }

        //Creating indexes and references for those tables the are not lookup tables using CASCADE";

        for (clm = 0; clm <= rTables.count()-1; clm++)
        {
            idx++;
            index = "INDEX fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + rTables[clm].toLower() ;
            indexes << index.left(64) + " (" + getForeignColumns(tables[pos],rTables[clm]) + ") , " << "\n";

            constraint = "CONSTRAINT fk" + QString::number(idx) + "_" + prefix + tables[pos].name.toLower() + "_" + rTables[clm].toLower();
            rels << constraint.left(64) << "\n";
            rels << "FOREIGN KEY (" + getForeignColumns(tables[pos],rTables[clm]) + ")" << "\n";
            rels << "REFERENCES " + prefix + rTables[clm].toLower() + " (" + getReferencedColumns(tables[pos],rTables[clm]) + ")" << "\n";
            if (!isRelatedTableLookUp(tables[pos].name))
                rels << "ON DELETE CASCADE " << "\n";
            else
                rels << "ON DELETE RESTRICT " << "\n";
            rels << "ON UPDATE NO ACTION," << "\n";
        }

        //Contatenate al different pieces of the create script into one SQL
        for (clm = 0; clm <= fields.count() -1;clm++)
        {
            sql = sql + fields[clm];
        }
        for (clm = 0; clm <= keys.count() -1;clm++)
        {
            keysql = keysql + keys[clm];
        }
        clm = keysql.lastIndexOf(",");
        keysql = keysql.left(clm) + ") , \n";

        sql = sql + keysql;

        for (clm = 0; clm <= indexes.count() -1;clm++)
        {
            sql = sql + indexes[clm];
        }
        for (clm = 0; clm <= rels.count() -1;clm++)
        {
            sql = sql + rels[clm];
        }
        clm = sql.lastIndexOf(",");
        sql = sql.left(clm);
        sql = sql + ")" + "\n ENGINE = InnoDB CHARSET=utf8; \n\n";

        //Append UUIDs triggers to the the
        UUIDStrm << "CREATE TRIGGER uudi_" + prefix+ tables[pos].name.toLower() + " BEFORE INSERT ON " + prefix + tables[pos].name.toLower() + " FOR EACH ROW SET new.rowuuid = uuid();\n";

        sqlCreateStrm << sql; //Add the final SQL to the DDL file

        //Create the inserts of the lookup tables values into the insert SQL
        if (tables[pos].lkpValues.count() > 0)
        {
            for (clm = 0; clm <= tables[pos].lkpValues.count()-1;clm++)
            {
                insertSQL = "INSERT INTO " + prefix + tables[pos].name.toLower() + " (";
                for (int pos2 = 0; pos2 <= tables[pos].fields.count()-2;pos2++)
                {
                    insertSQL = insertSQL + tables[pos].fields[pos2].name + ",";
                }
                insertSQL = insertSQL.left(insertSQL.length()-1) + ")  VALUES ('";

                insertSQL = insertSQL + tables[pos].lkpValues[clm].code.replace("'","`") + "',\"";
                insertSQL = insertSQL + tables[pos].lkpValues[clm].desc + "\");";
                sqlInsertStrm << insertSQL << "\n";


            }
        }
    }
    //Create the manifext file. If exist it get overwriten
    if (QFile::exists(xmlFile))
        QFile::remove(xmlFile);
    QFile file(xmlFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out.setCodec("UTF-8");
        outputdoc.save(out,1,QDomNode::EncodingFromTextStream);
        file.close();
    }
    else
        logout("Error: Cannot create xml manifest file");

    //Create the XMLCreare file. If exist it get overwriten
    if (QFile::exists(XMLCreate))
        QFile::remove(XMLCreate);
    QFile XMLCreateFile(XMLCreate);
    if (XMLCreateFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream outXMLCreate(&XMLCreateFile);
        outXMLCreate.setCodec("UTF-8");
        XMLSchemaStructure.save(outXMLCreate,1,QDomNode::EncodingFromTextStream);
        XMLCreateFile.close();
    }
    else
        logout("Error: Cannot create xml create file");

    //Create the XML Insert file. If exist it get overwriten
    if (QFile::exists(insertXML))
        QFile::remove(insertXML);
    QFile XMLInsertFile(insertXML);
    if (XMLInsertFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream outXMLInsert(&XMLInsertFile);
        outXMLInsert.setCodec("UTF-8");
        insertValuesXML.save(outXMLInsert,1,QDomNode::EncodingFromTextStream);
        XMLInsertFile.close();
    }
    else
        logout("Error: Cannot create xml insert file");
}

void appendUUIDs()
{
    int pos;
    for (pos = 0; pos <= tables.count()-1;pos++)
    {
        TfieldDef UUIDField;
        UUIDField.name = "rowuuid";
        UUIDField.desc = "Unique Row Identifier (UUID)";
        UUIDField.key = false;
        UUIDField.type = "varchar";
        UUIDField.xmlCode = "NONE";
        UUIDField.size = 80;
        UUIDField.decSize = 0;
        UUIDField.rTable = "";
        UUIDField.rField = "";
        tables[pos].fields.append(UUIDField);
    }
}

int main(int argc, char *argv[])
{
    QString title;
    title = title + "******************************************************************** \n";
    title = title + " * CSProToMySQL 2.0                                                 * \n";
    title = title + " * This tool generates a MySQL schema from a CSPro XML file.        * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2012                  * \n";
    title = title + " * CSProToMySQL is maintained by Carlos Quiros (cquiros@qlands.com) * \n";
    title = title + " ******************************************************************** \n";

    TCLAP::CmdLine cmd(title.toLatin1().constData(), ' ', "2.0");
    //Required arguments
    TCLAP::ValueArg<std::string> inputArg("x","inputXML","Input XML File",true,"","string");
    TCLAP::ValueArg<std::string> tableArg("t","mainTable","Main record in the XML that will become the main table",true,"","string");
    TCLAP::ValueArg<std::string> ddlArg("c","outputdml","Output DDL file. Default ./create.sql",false,"./create.sql","string");
    TCLAP::ValueArg<std::string> insertArg("i","outputinsert","Output insert file. Default ./insert.sql",false,"./insert.sql","string");
    TCLAP::ValueArg<std::string> metadataArg("m","outputmetadata","Output metadata file. Default ./metadata.sql",false,"./metadata.sql","string");
    TCLAP::ValueArg<std::string> prefixArg("p","prefix","Prefix for each table. _ is added to the prefix. Default no prefix",false,"","string");
    TCLAP::ValueArg<std::string> uuidFileArg("u","uuidfile","Output UUID trigger file",false,"./uuid-triggers.sql","string");
    TCLAP::ValueArg<std::string> impxmlArg("f","outputxml","Output xml manifest file. Default ./manifest.xml",false,"./manifest.xml","string");
    TCLAP::ValueArg<std::string> XMLCreateArg("C","xmlschema","Output XML schema file. Default ./create.xml",false,"./create.xml","string");
    TCLAP::ValueArg<std::string> insertXMLArg("I","xmlinsert","Output lookup values in XML format. Default ./insert.xml",false,"./insert.xml","string");

    TCLAP::SwitchArg includeYesNoLkpSwitch("l","inclkps","Add this argument to include Yes/No lookup tables", cmd, false);

    cmd.add(inputArg);
    cmd.add(tableArg);
    cmd.add(ddlArg);
    cmd.add(insertArg);
    cmd.add(metadataArg);
    cmd.add(prefixArg);
    cmd.add(uuidFileArg);
    cmd.add(impxmlArg);
    cmd.add(XMLCreateArg);
    cmd.add(insertXMLArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    QString input = QString::fromUtf8(inputArg.getValue().c_str());
    QString ddl = QString::fromUtf8(ddlArg.getValue().c_str());
    QString insert = QString::fromUtf8(insertArg.getValue().c_str());
    QString metadata = QString::fromUtf8(metadataArg.getValue().c_str());
    QString mainTable = QString::fromUtf8(tableArg.getValue().c_str());
    QString UUIDFile = QString::fromUtf8(uuidFileArg.getValue().c_str());
    QString xmlFile = QString::fromUtf8(impxmlArg.getValue().c_str());
    QString insertXML = QString::fromUtf8(insertXMLArg.getValue().c_str());
    QString xmlCreateFile = QString::fromUtf8(XMLCreateArg.getValue().c_str());
    prefix = QString::fromUtf8(prefixArg.getValue().c_str());




    if (prefix != "")
        prefix = prefix + "_";
    else
        prefix = "";

    bool ignoreYesNo = !includeYesNoLkpSwitch.getValue();


    QDomDocument doc("mydocument");
    QFile xmlfile(input);
    if (!xmlfile.open(QIODevice::ReadOnly))
        return 1;
    if (!doc.setContent(&xmlfile))
    {
        xmlfile.close();
        return 1;
    }
    xmlfile.close();

    QDomNodeList list;
    QDomNodeList list2;
    QDomElement item;
    QDomElement record;
    int pos;
    list = doc.elementsByTagName("IdItems");
    if (list.count() > 0)
    {
        item = list.item(0).toElement();
        list = item.elementsByTagName("Item");

        for (pos = 0; pos <= list.count()-1;pos++)
        {
            TfieldDef varmainID;
            item = list.item(pos).toElement();

            list2 = item.elementsByTagName("Label");
            varmainID.desc = list2.item(0).firstChild().nodeValue().toLower();
            list2 = item.elementsByTagName("Name");
            varmainID.name = list2.item(0).firstChild().nodeValue();
            list2 = item.elementsByTagName("Len");
            varmainID.size = list2.item(0).firstChild().nodeValue().toInt();
            list2 = item.elementsByTagName("DataType");
            if (list2.count() > 0)
            {
                if (list2.item(0).firstChild().nodeValue() == "Alpha")
                    varmainID.type = "Varchar";
                else
                    varmainID.type = "INT";
            }
            else
            {
                varmainID.type = "INT";
            }
            varmainID.key = true;
            //varmainID.join = true;
            varmainID.rField = "";
            varmainID.rTable = "";
            varmainID.decSize = 0;
            varmainID.xmlCode = varmainID.name;

            mainID.append(varmainID);
        }        
    }
    else
    {
        qDebug() << "Ups no ID items!";
        return 1;
    }
    QList<fieldDef> mainRelField;

    for (pos = 0; pos <= mainID.count()-1;pos++)
    {
        fieldDef varmainRelField;

        varmainRelField.name = mainID[pos].name;
        varmainRelField.desc = mainID[pos].desc;
        varmainRelField.size = mainID[pos].size;
        varmainRelField.type = mainID[pos].type;
        varmainRelField.key = true;
        varmainRelField.xmlCode = "NONE";
        varmainRelField.decSize = 0;
        varmainRelField.rTable = mainTable;
        varmainRelField.rField = mainID[pos].name;

        mainRelField.append(varmainRelField);

    }



    list = doc.elementsByTagName("Records");
    if (list.count() > 0)
    {
        int recordNo;
        recordNo = 0;
        record = list.item(0).firstChild().toElement(); //First record
        while (!record.isNull())
        {
            recordNo++;
            TtableDef table;
            table.isLookUp = false;
            table.pos = recordNo;
            list = record.elementsByTagName("Label");
            table.desc = list.item(0).firstChild().nodeValue();

            list = record.elementsByTagName("Name");
            table.name = list.item(0).firstChild().nodeValue().toLower();


            if (table.name.toLower() != mainTable.toLower())
            {
                table.fields.append(mainRelField);

                fieldDef childKey;
                childKey.name = table.name.toLower() + "_rowid";
                childKey.desc = "Unique Row ID";
                childKey.xmlCode = "NONE";
                childKey.size = 3;
                childKey.type = "INT";
                childKey.key = true;
                childKey.decSize = 0;
                childKey.rTable = "";
                childKey.rField = "";

                table.fields.append(childKey);
            }
            else
            {
                table.fields.append(mainID);
            }

            list = record.elementsByTagName("Items");

            table.fields.append(getFields(list.item(0).firstChild(),ignoreYesNo));

            if (tableExist(table.name) == false)
                tables.append(table);

            record = record.nextSibling().toElement();
        }
    }
    else
    {
        qDebug() << "Ups no Records!";
        return 1;
    }

    appendUUIDs();


    genSQL(mainTable.toLower(),ddl,insert,metadata,xmlFile,UUIDFile,xmlCreateFile,insertXML);


    return 0;
}
