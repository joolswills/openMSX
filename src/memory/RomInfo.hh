// $Id$

#ifndef __ROMINFO_HH__
#define __ROMINFO_HH__

#include "RomTypes.hh"
#include <string>

class Rom;
class Device;

using namespace std;

class RomInfo
{
	public:
		RomInfo(const string &nid, const string &nyear,
		        const string &ncompany, const string &nremark,
		        const MapperType &nmapperType);
		
		const string &getTitle()          const { return title; }
		const string &getYear()           const { return year; }
		const string &getCompany()        const { return company; }
		const string &getRemark()         const { return remark; }
		const MapperType &getMapperType() const { return mapperType; }

		static RomInfo *fetchRomInfo(
		        const Rom *rom, const Device &deviceConfig);
		static MapperType nameToMapperType(const string &name);
		void print();

	private:
		/** Search for a ROM in the ROM database.
		  * @param rom ROM to look up.
		  * @return The information found in the database,
		  * 	or NULL if the given ROM is not in the database.
		  */
		static RomInfo *searchRomDB(const Rom *rom);
		static MapperType guessMapperType(const Rom *rom);
		
		string title;
		string year;
		string company;
		string remark;
		MapperType mapperType;
};

#endif
