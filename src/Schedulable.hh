// $Id$

#ifndef SCHEDULABLE_HH
#define SCHEDULABLE_HH

#include "noncopyable.hh"
#include <string>

namespace openmsx {

class EmuTime;
class Scheduler;

/**
 * Every class that wants to get scheduled at some point must inherit from
 * this class.
 */
class Schedulable : private noncopyable
{
public:
	/**
	 * When the previously registered syncPoint is reached, this
	 * method gets called. The parameter "userData" is the same
	 * as passed to setSyncPoint().
	 */
	virtual void executeUntil(const EmuTime& time, int userData) = 0;

	/**
	 * Just before the the Scheduler is deleted, it calls this method of
	 * all the Schedulables that are still registered.
	 * If you override this method you should unregister this Schedulable
	 * in the implementation. The default implementation just prints a
	 * diagnostic (and soon after the Scheduler will trigger an assert that
	 * there are still regsitered Schedulables.
	 * Normally there are easier ways to unregister a Schedulable. ATM this
	 * method is only used in AfterCommand (because it's not part of the
	 * MSX machine).
	 */
	virtual void schedulerDeleted();

	/**
	 * This method is only used to print meaningfull debug messages
	 */
	virtual const std::string& schedName() const = 0;

	Scheduler& getScheduler() const;

	/** Convenience method:
	  * This is the same as getScheduler().getCurrentTime(). */
	const EmuTime& getCurrentTime() const;

protected:
	explicit Schedulable(Scheduler& scheduler);
	virtual ~Schedulable();

	void setSyncPoint(const EmuTime& timestamp, int userData = 0);
	void removeSyncPoint(int userData = 0);
	void removeSyncPoints();
	bool pendingSyncPoint(int userData = 0);

private:
	Scheduler& scheduler;
};

} // namespace openmsx

#endif
