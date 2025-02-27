#ifndef RESAMPLEDSOUNDDEVICE_HH
#define RESAMPLEDSOUNDDEVICE_HH

#include "SoundDevice.hh"
#include "Observer.hh"
#include <memory>

namespace openmsx {

class MSXMotherBoard;
class ResampleAlgo;
class Setting;
template<typename T> class EnumSetting;

class ResampledSoundDevice : public SoundDevice, protected Observer<Setting>
{
public:
	enum ResampleType { RESAMPLE_HQ, RESAMPLE_LQ, RESAMPLE_BLIP };

	/** Note: To enable various optimizations (like SSE), this method is
	  * allowed to generate up to 3 extra sample.
	  * @see SoundDevice::updateBuffer()
	  */
	bool generateInput(float* buffer, unsigned num);

protected:
	ResampledSoundDevice(MSXMotherBoard& motherBoard, string_view name,
	                     string_view description, unsigned channels,
	                     unsigned inputSampleRate, bool stereo);
	~ResampledSoundDevice();

	// SoundDevice
	void setOutputRate(unsigned sampleRate) override;
	bool updateBuffer(unsigned length, float* buffer,
	                  EmuTime::param time) override;

	// Observer<Setting>
	void update(const Setting& setting) override;

	void createResampler();

private:
	EnumSetting<ResampleType>& resampleSetting;
	std::unique_ptr<ResampleAlgo> algo;
};

} // namespace openmsx

#endif
