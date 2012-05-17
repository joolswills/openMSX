// $Id$

#ifndef IDEDEVICEFACTORY_HH
#define IDEDEVICEFACTORY_HH

#include <memory>

namespace openmsx {

class IDEDevice;
class MSXMotherBoard;
class DeviceConfig;

namespace IDEDeviceFactory
{
	std::auto_ptr<IDEDevice> create(MSXMotherBoard& motherBoard,
	                                const DeviceConfig& config);
}

} // namespace openmsx

#endif
