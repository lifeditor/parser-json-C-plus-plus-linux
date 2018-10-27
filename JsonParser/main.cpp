#include "rapidjson/document.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <experimental/filesystem>
#include <algorithm>
#include <dirent.h>

using namespace rapidjson;
using namespace std;

const string UP_DIRECTORY = "..";
const string CURRENT_DIRECTORY = ".";
const string SEPARATOR = "/";
const string CSV_DELIMITER = ";";

const string MSG_ERROR_DIRECTORY_OPEN = "Error opening : ";
const string MSG_ERROR_FILE_OPERATION = "!!!Warning!!! Error detected! ";
const string MSG_BEGIN_FILE_OPERATION = "Starting files operations...";
const string MSG_END_FILE_OPERATION = "All files operations finished.";

const string FILE_SOURCE_DIRECTORY = "./test";
const string FILE_SOURCE_EXTENSION = ".json";
const string FILE_DESTINATION_EXTENSION = ".csv";

const string JSON_BAD_FORMAT = "BAD FORMAT";
const string JSON_NO_ODDS = "NO ODDS";

const char* KEY_DELTA = "delta";
const char* KEY_BM = "bm";
const char* KEY_DESC = "desc";
const char* KEY_ID = "id";
const char* KEY_ODDS = "odds";


vector<string> getOddsData(const string json) {
	vector<string> oddsData;
	stringstream stringStream;

	try {
		Document document;

		if (document.Parse(json.c_str()).HasParseError())
			throw JSON_BAD_FORMAT;

		if (!document.HasMember(KEY_DELTA))
			throw JSON_BAD_FORMAT;

		const Value & delta = document[KEY_DELTA];

		if (!delta.HasMember(KEY_BM))
			throw JSON_BAD_FORMAT;

		const Value & bm = delta[KEY_BM];

		for (Value::ConstMemberIterator itr = bm.MemberBegin(); itr != bm.MemberEnd(); ++itr) {
			const Value & bmdata = itr->value;

			if (!bmdata.HasMember(KEY_DESC))
				throw JSON_BAD_FORMAT;

			const Value & desc = bmdata[KEY_DESC];

			if (!desc.HasMember(KEY_ID))
				throw JSON_BAD_FORMAT;

			string id =  to_string( desc[KEY_ID].GetInt() );

			for (Value::ConstMemberIterator itr = bmdata.MemberBegin(); itr != bmdata.MemberEnd(); ++itr)
				if (itr->name != KEY_DESC) {
					const Value & oddsdata = itr->value;

					if (oddsdata.HasMember(KEY_ODDS)) {
						const Value & odds = oddsdata[KEY_ODDS];

						for (Value::ConstMemberIterator itr = odds.MemberBegin(); itr != odds.MemberEnd(); ++itr)
							oddsData
							.push_back(id + CSV_DELIMITER + itr->name.GetString() + CSV_DELIMITER + itr->value.GetString());
					}
				}
		}
    }
    catch (...) {
    	oddsData.clear();
    	oddsData.push_back(JSON_BAD_FORMAT);
    }

    if (oddsData.size() == 0) {
    	oddsData.push_back(JSON_NO_ODDS);
    }

	return oddsData;
}


string getFileExtension (string fileName) {
	int dotPos = fileName.find_last_of(".");

	return dotPos > 0 ? fileName.substr(dotPos) : "";
}


string getPath(initializer_list<string> parts) {
    string pathTmp {};
    string separator = "";

    for (auto & part: parts) {
        pathTmp.append(separator).append(part);
        separator = SEPARATOR;
    }

    return pathTmp;
}


vector<string> getDirectoryFiles(const string & dir, const vector<string> & extensions) {
    vector<string> files;
    shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });

    if (!directory_ptr)
        throw system_error(error_code(errno, system_category()), MSG_ERROR_DIRECTORY_OPEN + dir);

    struct dirent *dirent_ptr;

    while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr) {
        const string fileName {dirent_ptr->d_name};

        if (dirent_ptr->d_type == DT_DIR) {
            if (CURRENT_DIRECTORY != fileName && UP_DIRECTORY != fileName) {
                auto subFiles = getDirectoryFiles(getPath({dir, fileName}), extensions);
                files.insert(end(files), begin(subFiles), end(subFiles));
            }
        }
        else if (dirent_ptr->d_type == DT_REG &&
        		 find(extensions.begin(), extensions.end(), getFileExtension(fileName)) != extensions.end()
		)
        	files.push_back(getPath({dir, fileName}));
    }

    return files;
}


int main() {
	cout << MSG_BEGIN_FILE_OPERATION << endl;

    try {
    	for (auto & fileName: getDirectoryFiles(FILE_SOURCE_DIRECTORY, {FILE_SOURCE_EXTENSION})) {
    		string line;
    		stringstream json;

    		ifstream fileSource (fileName);
    		if (fileSource.is_open()) {
    			while (getline(fileSource, line))
    				json<<line;

    			ofstream fileDestination (fileName.replace(fileName
    						.find(FILE_SOURCE_EXTENSION), FILE_SOURCE_EXTENSION.length(), FILE_DESTINATION_EXTENSION));
    			if (fileDestination) {
    				for (auto & oddsData: getOddsData(json.str()))
    					fileDestination << oddsData << endl;
    			}
    			fileDestination.close();
    		}
    		fileSource.close();
    	}
    	cout << MSG_END_FILE_OPERATION << endl;
    }
    catch (exception & ex) {
    	cout << MSG_ERROR_FILE_OPERATION << ex.what()<< endl;
    }

    return 0;
}
