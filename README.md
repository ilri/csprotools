# CSPro Tools
CSPro Tools is a toolbox for processing [CSPro](https://www.census.gov/population/international/software/cspro/) survey data into MySQL databases. The toolbox relies on on [META](https://github.com/ilri/meta) for storing the dictionary information. CSPro Tools comprises of four command line tools performing different tasks. The toolbox is cross-platform and can be build in Windows, Linux and Mac. 

## The toolbox
- 
###DCFToXML
DCFToXML converts a CSPro dictionary file (DCF) into its representation as XML for better application usage.

 The tool produces the following output files:
 - output.xml. An XML representation of the CSPro dictionary file. **This file is used in subsequest tools.**

  #### *Parameters*
  - i - Input CSPro DCF file.
  - o - Output XML file. 
  
 ### *Example*
  ```sh
$ ./dcftoxml -i my_input_dcf_file.dcf -o my_output_xml_file.xml
```

- 
###CSProToMySQL
CSProToMySQL converts a CSPro Dictionary file (in XML format) into a relational MySQL schema. 

 CSProToMySQL creates a complete relational schemas with the following features:
 - A record can be identified as the main table. The main table will be linked to the rest of tables.
 - Value lists are stored in lookup tables. Lookup tables are then related to main tables.
 - Records are converted to tables.
 - Records and variable names are stored in METAS's dictionary tables.
 - Duplicated value lists are avoided.
 - Yes/No selects are ignored

 The tool produces the following output files:
 - create.sql. A [DDL](http://en.wikipedia.org/wiki/Data_definition_language) script containing all data structures, indexes and relationships.
 - insert.sql. A [DML](http://en.wikipedia.org/wiki/Data_manipulation_language) script that inserts all the value list values in the lookup tables.
 - uuid-triggers.sql. A script containing code for storing each row in each table with an [Universally unique identifier](http://en.wikipedia.org/wiki/Universally_unique_identifier) (UUID).
 - metadata.sql. A DML script that inserts the name of all tables and variables in META's dictionary tables.

  #### *Parameters*
  - x - Input CSPro XML file.
  - t - Main record in CSPro, usually it contains the main survey variable. This will become the main table in the schema.
  - p - Table prefix. A prefix that can be added to each table. This is useful if a common schema is used to store different surveys.
  - c - Output DDL file. "create.sql" by default.
  - i - Output DML file. "insert.sql" by default.
  - u - Output UUID trigger file. "uuid-triggers.sql" by default.  
  - m - Output metadata file. "metadata.sql" by default.
  
 ### *Example*
  ```sh
$ ./csprotomysql -x my_input_xml_file.xml -t maintable
```

- 
###CSProDatToSQlite
CSProDatToSQlite converts a CSPro data file (DAT) into a [SQLite](https://www.sqlite.org/) file. CSPro stores the data into a "flat" text file and is very complicated to control which records are imported or not into the database. To solve this the data is converted into SQLite which makes very easy to read and manipulate the data. Think about the SQlite file as a temporary repository for the CSPro data before it reach the MySQL database.

 The tool produces the following output files:
 - output.sqlite. The same CSPro data but in SQLite format with some extra controlling fields.
 - output.csv. A log file in CSV format with any errors encountered in the conversion. **These errors are mainly caused by using a dictionary file that does not match the data file.**

 #### *Parameters*
  - x - Input CSPro XML file. 
  - d - Input CSPro DAT file.
  - s - Output SQLite data file. "output.sqlite" by default
  - l - Output log file. "output.csv" by default
  - w - Whether to overwrite the log file. False by default

 ### *Example*
  ```sh
$ ./csprodattosqlite -x my_input_xml_file.xml -d my_input_dat_file.dat -s my_output_sqlite_file.sqlite -l my_output_log_file.csv
```

- 
###GenImportScript
GenImportScript generates a Python scripts that imports the data from a CSPro SQLite data file into the MySQL database. Why a python script? With CSPro, we found that in some cases it is necessary to modify the way the data is imported into the database. For example, if a table should be imported before another one. A python script allow easy modifications of the import process without the need to compiling an application. 

 The tool produces the following output files:
 - import.py. The script that moves the data from SQLite to MySQL.

 #### *Parameters*
  - x - Input CSPro XML file.
  - t - Main record in CSPro, usually it contains the main survey variable. This will become the main table in the database.
  - p - Table prefix. A prefix that can be added to each table. This is useful if a common schema is used to store different tables.
  - o - Output Python script file. "output.csv" by default

 ### *Example*
  ```sh
$ ./genimportscript -x my_input_xml_file.xml -t maintable -o my_import_python_file
```

- 
###Import Script (Generated Python file)
The import script imports the data from a CSPro SQLite data file into the MySQL database. The scrip imports one table at a time. 

 The tool produces the following output files:
 - output.csv. A log file containing any errors in the importation process.

 #### *Parameters*
  - h - MySQL host server. Default is localhost.
  - u - User who has insert access to the schema.
  - p - Password of the user.
  - s - Target schema in the MySQL server.
  - i - Input CSPro SQLite data file.
  - t - Table to be imported.
  - o - Output log file. "output.csv" by default.
  - w - Overwrite log file. False by default.

 ### *Example*
  ```sh
$ python my_import_python_file -h localhost -u my_user -p my_password -s my_schema -t my_table_to_import -o my_log_file_.csv
```

## Technology
ODK Tools was built using:

- [C++](https://isocpp.org/), a general-purpose programming language.
- [Qt 4.8.x](https://www.qt.io/), a cross-platform application framework.
- [Python 2.7.x](https://www.python.org/), a widely used general-purpose programming language.
- [MySQLdb](http://mysql-python.sourceforge.net/MySQLdb.html), a thread-compatible interface to the popular MySQL database server that provides the Python database API. Install it with [pip](https://pip.pypa.io/en/stable/) "pip install MySQL-python"
- [TClap](http://tclap.sourceforge.net/), a small, flexible library that provides a simple interface for defining and accessing command line arguments.


## Building and testing
To build this site for local viewing or development:

    $ git clone https://github.com/ilri/csprotools.git
    $ cd csprotools
    $ qmake
    $ make

## Author
Carlos Quiros (c.f.quiros_at_cgiar.org / cquiros_at_qlands.com)

## License
This repository contains the code of:

- [TClap](http://tclap.sourceforge.net/) which is licensed under the [MIT license](https://raw.githubusercontent.com/twbs/bootstrap/master/LICENSE).

Otherwise, the contents of this application is [GPL V3](http://www.gnu.org/copyleft/gpl.html). 
 
