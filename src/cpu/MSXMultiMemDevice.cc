#include "MSXMultiMemDevice.hh"
#include "DummyDevice.hh"
#include "MSXCPUInterface.hh"
#include "StringOp.hh"
#include "likely.hh"
#include "unreachable.hh"
#include "xrange.hh"
#include <algorithm>
#include <cassert>

namespace openmsx {

MSXMultiMemDevice::Range::Range(
		unsigned base_, unsigned size_, MSXDevice& device_)
	: base(base_), size(size_), device(&device_)
{
}

bool MSXMultiMemDevice::Range::operator==(const Range& other) const
{
	return (base   == other.base) &&
	       (size   == other.size) &&
	       (device == other.device);
}


MSXMultiMemDevice::MSXMultiMemDevice(const HardwareConfig& hwConf)
	: MSXMultiDevice(hwConf)
{
	// add sentinel at the end
	ranges.push_back(Range(0x0000, 0x10000, getCPUInterface().getDummyDevice()));
}

MSXMultiMemDevice::~MSXMultiMemDevice()
{
	assert(empty());
}

static bool isInside(unsigned x, unsigned start, unsigned size)
{
	return (x - start) < size;
}

static bool overlap(unsigned start1, unsigned size1,
                    unsigned start2, unsigned size2)
{
	return (isInside(start1,             start2, size2)) ||
	       (isInside(start1 + size1 - 1, start2, size2));
}

bool MSXMultiMemDevice::canAdd(int base, int size)
{
	for (auto i : xrange(ranges.size() - 1)) {
		if (overlap(base, size, ranges[i].base, ranges[i].size)) {
			return false;
		}
	}
	return true;
}

void MSXMultiMemDevice::add(MSXDevice& device, int base, int size)
{
	assert(canAdd(base, size));
	ranges.insert(ranges.begin(), Range(base, size, device));
}

void MSXMultiMemDevice::remove(MSXDevice& device, int base, int size)
{
	auto it = find(ranges.begin(), ranges.end(),
	               Range(base, size, device));
	assert(it != ranges.end());
	ranges.erase(it);
}

bool MSXMultiMemDevice::empty() const
{
	return ranges.size() == 1;
}

std::vector<MSXDevice*> MSXMultiMemDevice::getDevices() const
{
	std::vector<MSXDevice*> result;
	for (auto i : xrange(ranges.size() - 1)) {
		result.push_back(ranges[i].device);
	}
	return result;
}

std::string MSXMultiMemDevice::getName() const
{
	assert(!empty());
	StringOp::Builder result;
	result << ranges[0].device->getName();
	for (auto i : xrange(size_t(1), ranges.size() - 1)) {
		result << "  " << ranges[i].device->getName();
	}
	return result;
}

const MSXMultiMemDevice::Range& MSXMultiMemDevice::searchRange(unsigned address) const
{
	for (auto& r : ranges) {
		if (isInside(address, r.base, r.size)) {
			return r;
		}
	}
	UNREACHABLE;
}

MSXDevice* MSXMultiMemDevice::searchDevice(unsigned address) const
{
	return searchRange(address).device;
}

byte MSXMultiMemDevice::readMem(word address, EmuTime::param time)
{
	return searchDevice(address)->readMem(address, time);
}

byte MSXMultiMemDevice::peekMem(word address, EmuTime::param time) const
{
	return searchDevice(address)->peekMem(address, time);
}

void MSXMultiMemDevice::writeMem(word address, byte value, EmuTime::param time)
{
	searchDevice(address)->writeMem(address, value, time);
}

const byte* MSXMultiMemDevice::getReadCacheLine(word start) const
{
	assert((start & CacheLine::HIGH) == start); // start is aligned
	// Because start is aligned we don't need to wory about the begin
	// address of the range. But we must make sure the end of the range
	// doesn't only fill a partial cacheline.
	const auto& range = searchRange(start);
	if (unlikely(((range.base + range.size) & CacheLine::HIGH) == start)) {
		// The end of this memory device only fills a partial
		// cacheline. This can't be cached.
		return nullptr;
	}
	return searchDevice(start)->getReadCacheLine(start);
}

byte* MSXMultiMemDevice::getWriteCacheLine(word start) const
{
	assert((start & CacheLine::HIGH) == start);
	const auto& range = searchRange(start);
	if (unlikely(((range.base + range.size) & CacheLine::HIGH) == start)) {
		return nullptr;
	}
	return searchDevice(start)->getWriteCacheLine(start);
}

} // namespace openmsx
