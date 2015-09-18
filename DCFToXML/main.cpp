#include <tclap/CmdLine.h>
#include <QtXml>
#include <QFile>


QDomDocument doc;
QDomElement root;
QDomElement dict;
QDomElement level;
QDomElement iditems;
QDomElement items;
QDomElement item;
QDomElement records;
QDomElement record;
QDomElement valueSet;
QDomElement valueSetValues;

bool inDict;
bool inLevel;
bool inIdItems;
bool inItem;
bool inRecord;
bool inValueSet;
bool createValues;
bool createItems;
bool createRecords;

void appendValue(QDomElement parent, QString data)
{
    // This function appends the value of the value set.
    // It replaces the ; with |

    int pos;
    pos = data.indexOf("=");
    QString temp;
    temp = data.right(data.length()-pos-1);
    temp = temp.replace(";","|");

    QDomElement varname;
    QDomText varvalue;

    varname = doc.createElement("Value");
    parent.appendChild(varname);
    varvalue = doc.createTextNode(temp);
    varname.appendChild(varvalue);

}

void appendVar(QDomElement parent, QString data)
{
    //This appends a variable to a parent
    //In CSPro varieables are separated by  =
    // We remove the = and the node name is the variable
    // while the value becomes the value of the node

    int pos;
    pos = data.indexOf("=");
    QString var;
    QString value;
    var = data.left(pos);
    value = data.right(data.length()-pos-1);

    QDomElement varname;
    QDomText varvalue;

    varname = doc.createElement(var);
    parent.appendChild(varname);
    varvalue = doc.createTextNode(value);
    varname.appendChild(varvalue);

}

void addToXML(QString data)
{
    /*
        This fuction will parse the data from the CSPro
        into XML nodes. There are several controling bool
        variables to knows where in the CSPro data data is
        and how is its parent
    */
    bool section;
    section = false;

    QString currData;
    currData = data;

    if (data == "[Dictionary]")
    {
        section = true;
        inDict = true;
        inLevel = false;
        inIdItems = false;
        inItem = false;
        inRecord = false;
        inValueSet = false;
        createRecords = true;
        dict = doc.createElement("Dictionary");
        root.appendChild(dict);
    }
    if (data == "[Level]")
    {
        section = true;
        inDict = false;
        inLevel = true;
        inIdItems = false;
        inRecord = false;
        inItem = false;
        inValueSet = false;
        level = doc.createElement("Level");
        dict.appendChild(level);
    }
    if (data == "[IdItems]")
    {
        section = true;
        inDict = false;
        inLevel = false;
        inIdItems = true;
        inRecord = false;
        inItem = false;
        inValueSet = false;
        createItems = true;
        iditems = doc.createElement("IdItems");
        dict.appendChild(iditems);
    }
    if (data == "[Item]")
    {
        section = true;
        inDict = false;
        inLevel = false;
        inItem = true;
        inValueSet = false;
        if (createItems)
        {
            if (inIdItems)
            {
                items = doc.createElement("Items");
                iditems.appendChild(items);
            }
            if (inRecord)
            {
                items = doc.createElement("Items");
                record.appendChild(items);
            }
            createItems = false;
        }

        item = doc.createElement("Item");
        items.appendChild(item);
    }
    if (data == "[Record]")
    {
        section = true;
        inDict = false;
        inLevel = false;
        inIdItems = false;
        inItem = false;
        inRecord = true;
        inValueSet = false;
        createItems = true;
        if (createRecords)
        {
            records = doc.createElement("Records");
            dict.appendChild(records);
            createRecords = false;
        }
        record = doc.createElement("Record");
        records.appendChild(record);
    }
    if (data == "[ValueSet]")
    {
        section = true;
        inDict = false;
        inLevel = false;
        inItem = false;
        inValueSet = true;
        valueSet = doc.createElement("ValueSet");
        item.appendChild(valueSet);
        createValues = true;
    }
    if (section == false)
    {
        if (!inValueSet)
        {
            if (inDict)
            {
                appendVar(dict,data);
            }
            if (inLevel)
            {
                appendVar(level,data);
            }
            if ((inIdItems) && (!inItem))
            {
                appendVar(iditems,data);
            }
            if ((inIdItems) && (inItem))
            {
                appendVar(item,data);
            }
            if ((inRecord) && (!inItem))
            {
                appendVar(record,data);
            }
            if ((inRecord) && (inItem))
            {
                appendVar(item,data);
            }
        }
        else
        {
            if (!data.contains("Value="))
                appendVar(valueSet,data);
            else
            {
                if (createValues)
                {
                    valueSetValues = doc.createElement("ValueSetValues");
                    valueSet.appendChild(valueSetValues);
                    appendValue(valueSetValues,data);
                    createValues = false;
                }
                else
                    appendValue(valueSetValues,data);
            }
        }
    }
}

int createXML(QString fileName)
{
    //This will save the XML document into a file
    if (QFile::exists(fileName))
        QFile::remove(fileName);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out.setCodec("UTF-8");

        //out << doc.toString();
        doc.save(out,1,QDomNode::EncodingFromTextStream);

        file.close();
        return 0;
    }
    else
        return 1;
}

void printLog(QString message)
{
    QString temp;
    temp = message + "\n";
    printf(temp.toLocal8Bit().data());
}

int main(int argc, char *argv[])
{
    QString title;
    title = title + "****************************************************************** \n";
    title = title + " * DictToXML 1.0                                                  * \n";
    title = title + " * This tool generates an XML structure of a CSPro dictionary     * \n";
    title = title + " * file. The XML has the same data as the DCF file but using      * \n";
    title = title + " * XML tags. The purpose of an XML structure is to easily read    * \n";
    title = title + " * dictionary information or to quickly alter value lists.        * \n";
    title = title + " * The XMLToDict do the oposite.                                  * \n";
    title = title + " * This tool is part of CSPro Tools (c) ILRI, 2012                * \n";
    title = title + " ****************************************************************** \n";

    TCLAP::CmdLine cmd(title.toLatin1().constData(), ' ', "1.0 (Beta 1)");
    //Required arguments
    TCLAP::ValueArg<std::string> inputArg("i","input","Input CSPro DCF File",true,"","string");
    TCLAP::ValueArg<std::string> outputArg("o","output","Output XML file. Default ./output.xml",false,"./output.xml","string");

    cmd.add(inputArg);
    cmd.add(outputArg);
    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command

    QString input = QString::fromUtf8(inputArg.getValue().c_str());
    QString output = QString::fromUtf8(outputArg.getValue().c_str());

    QFile file(input);

    if (!file.exists())
    {
        printLog("The input DCF file does not exits");
        return 1;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        printLog("Cannot open DCF file");
        return 1;
    }

    doc = QDomDocument("CSProXMLFile");

    root = doc.createElement("CSProXML");
    root.setAttribute("version", "1.0");
    doc.appendChild(root);

    QTextStream in(&file);
    int pos;
    pos = 1;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.isEmpty())
            addToXML(line); //Parse the line
        pos++;
    }

    if (createXML(output) != 0)
    {
        printLog("Error saving XML file");
        return 1;
    }

    printLog("Done");
    return 0;
}
