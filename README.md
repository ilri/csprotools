# CSPro Tools 2.0
CSPro Tools is a toolbox for processing [CSPro](https://www.census.gov/population/international/software/cspro/) survey data into MySQL databases. The toolbox relies on on [META](https://github.com/ilri/meta) for storing the dictionary information. CSPro Tools comprises of three command line tools performing different tasks. The toolbox is cross-platform and can be build in Windows, Linux and Mac.

This version differs from version 1 in the following:
 - JSON files are used to import the data into MySQL. Instead of using SQLite
 - The data is imported using JSONToMySQL (from ODKTools). Instead of using a generated python script.

## The toolbox

### DCFToXML
DCFToXML converts a CSPro dictionary file (DCF) into its representation as XML for better application usage.

 The tool produces the following output files:
 - output.xml. An XML representation of the CSPro dictionary file. **This file is used in subsequent tools.**

  #### *Parameters*
  - i - Input CSPro DCF file.
  - o - Output XML file.

 ### *Example*
  ```sh
$ ./dcftoxml -i my_input_dcf_file.dcf -o my_output_xml_file.xml
```


### CSProToMySQL
CSProToMySQL converts a CSPro Dictionary file (in XML format) into a relational MySQL schema.

 CSProToMySQL creates a complete relational schema with the following features:
 - A record can be identified as the main table. The main table will be linked to the rest of tables.
 - Value lists are stored in lookup tables. Lookup tables are then related to main tables.
 - Records are converted to tables.
 - Records and variable names are stored in METAS's dictionary tables.
 - Duplicated value lists are avoided.
 - Yes/No selects are ignored

 The tool produces the following output files:
 - create.sql: A [DDL](http://en.wikipedia.org/wiki/Data_definition_language) script containing all data structures, indexes and relationships.
 - insert.sql: A [DML](http://en.wikipedia.org/wiki/Data_manipulation_language) script that inserts all the select and multi-select values in the lookup tables.
 - uuid-triggers.sql: A script containing code for storing each row in each table with an [Universally unique identifier](http://en.wikipedia.org/wiki/Universally_unique_identifier) (UUID).
 - metadata.sql: A DML script that inserts the name of all tables and variables in META's dictionary tables.
 - manifest.xml: This file maps each variable in the CSPro survey with its corresponding field in a table in the MySQL database. **This file is used in subsequent tools.**
 - create.xml: This is a XML representation of the schema. Used by utilities compareCreateXML & createFromXML.
 - insert.xml: This is a XML representation of the lookup tables and values. Used by utilities compareInsertXML & insertFromXML.

  #### *Parameters*
  - x - Input CSPro XML file.  
  - t - Main record in CSPro, usually it contains the main survey variable. This will become the main table in the schema.
  - p - Table prefix. A prefix that can be added to each table. This is useful if a common schema is used to store different surveys.
  - c - Output DDL file. "create.sql" by default.
  - i - Output DML file. "insert.sql" by default.
  - u - Output UUID trigger file. "uuid-triggers.sql" by default.  
  - m - Output metadata file. "metadata.sql" by default.  
  - f - Output manifest file. "manifest.xml" by default  
  - I - Output lookup tables and values in XML format. "insert.xml" by default.
  - C - Output schema as in XML format. "create.xml" by default


 ### *Example*
  ```sh
$ ./csprotomysql -x my_input_csppro_xml_file.xml -t maintable
```

### CSProDatToJSON
CSProDatToJSON converts a CSPro data file (DAT) into [JSON](https://en.wikipedia.org/wiki/JSON) files. CSPro stores the data into a "flat" text file and is very complicated to control which case are imported or not into the database. To solve this each case is exported to a different JSON file. JSON files are then imported with [JSONToMySQL](https://github.com/ilri/odktools)

 In addition to JSON files the tool produces the following output file:
 - output.csv. A log file in CSV format with any errors encountered in the conversion. **These errors are mainly caused by using a dictionary file that does not match the data file.**

 #### *Parameters*
  - x - Input CSPro XML file.
  - d - Input CSPro DAT file.
  - o - Output directory for the JSON files.
  - l - Output log file. "output.csv" by default
  - t - Main record in CSPro.
  - v - Version of CSPro. The DAT files has changed from CSPro 5 to CSPro 6. You need to specify the version of CSPro used to **COLLECT** the data.
  - w - Whether to overwrite the log file. False by default
  - W - By default CSProDatToJSON does not overwrite a JSON file if it exists. Use this to change this behavior.

 ### *Example*
  ```sh
$ ./csprodattojson -x my_input_csppro_xml_file.xml -d my_input_date_file.dat -v 5 -o /where/to/save/the/jsons -t my_main_record
```

## Technology
CSPro Tools was built using:

- [C++](https://isocpp.org/), a general-purpose programming language.
- [Qt 5.6.x](https://www.qt.io/), a cross-platform application framework.
- [TClap](http://tclap.sourceforge.net/), a small, flexible library that provides a simple interface for defining and accessing command line arguments.
- [QJSON](https://github.com/flavio/qjson), a qt-based library that maps JSON data to QVariant objects. *(Included in source code)*
- [CMake](http://www.cmake.org/), a cross-platform free and open-source software for managing the build process of software using a compiler-independent method.


## Building and testing
To build this site for local viewing or development:

    $ git clone https://github.com/ilri/csprotools.git
    $ cd csprotools
    $ git submodule update --init --recursive
    $ cd 3rdparty/qjson
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install
    $ cd ../../..
    $ qmake
    $ make

## Author
Carlos Quiros (c.f.quiros_at_cgiar.org / cquiros_at_qlands.com)

## License
This repository contains the code of:

- [TClap](http://tclap.sourceforge.net/) which is licensed under the [MIT license](https://raw.githubusercontent.com/twbs/bootstrap/master/LICENSE).
- [QJSON](https://github.com/flavio/qjson) which is licensed under the [GNU Lesser General Public License version 2.1](http://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html)

Otherwise, the contents of this application is [LGPL V3](https://www.gnu.org/licenses/lgpl-3.0.en.html).
