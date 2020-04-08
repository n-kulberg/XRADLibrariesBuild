// file TrackedObject.h
//--------------------------------------------------------------
#ifndef __TrackedObject_h
#define __TrackedObject_h
//--------------------------------------------------------------

#include <XRADGUI/XRAD.h>
#include <set>

XRAD_BEGIN

//--------------------------------------------------------------

class TrackedObject
{
	public:
		TrackedObject(): value(0)
		{
			tracker.insert(this);
		}
		TrackedObject(int value): value(value)
		{
			tracker.insert(this);
		}
		TrackedObject(const TrackedObject &other): TrackedObject(other.value)
		{
		}
		TrackedObject &operator=(const TrackedObject &other)
		{
			value = other.value;
			return *this;
		}
		~TrackedObject()
		{
			if (!tracker.count(this))
			{
				Error("~TrackedObject(): this object is not contained in the tracker base.");
			}
			else
			{
				tracker.erase(this);
			}
		}
	public:
		int value;
	public:
		static size_t tracker_size() { return tracker.size(); }
	private:
		static std::set<TrackedObject*> tracker;
};

//--------------------------------------------------------------

XRAD_END

//--------------------------------------------------------------
#endif // __TrackedObject_h
