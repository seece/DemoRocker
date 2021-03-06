#pragma once

#ifndef SYNCTRACK_H
#define SYNCTRACK_H

#include <string>
#include <map>
#include <cassert>

class SyncTrack {
public:
	SyncTrack(const std::string &name, std::string &displayName) :
		name(name), displayName(displayName), active(false)
	{
	}

	struct TrackKey {
		bool operator ==(const TrackKey &k) const
		{
			return row == k.row &&
				value == k.value &&
				type == k.type;
		}

		int row;
		float value;
		enum KeyType {
			STEP,   /* stay constant */
			LINEAR, /* lerp to the next value */
			SMOOTH, /* smooth curve to the next value */
			RAMP,   /* ramp up */
			KEY_TYPE_COUNT
		} type;
	};

	void setKey(const TrackKey &key)
	{
		if (isKeyFrame(key.row)) {
			const TrackKey oldKey = keys[key.row];
			keys[key.row] = key;
			//emit keyFrameChanged(key.row, oldKey);
		}
		else {
			keys[key.row] = key;
			//emit keyFrameAdded(key.row);
		}
	}

	void removeKey(int row)
	{
		assert(keys.find(row) != keys.end());
		const TrackKey oldKey = keys[row];
		//keys.remove(row);
		//emit keyFrameRemoved(row, oldKey);
	}

	bool isKeyFrame(int row) const
	{
		assert(!"implement");
		/*
		auto it = keys.lowerBound(row);
		return it != keys.end() && it.key() == row;
		*/
	}

	TrackKey getKeyFrame(int row) const
	{
		assert(isKeyFrame(row));
		assert(!"implement");
		/*
		QMap<int, TrackKey>::const_iterator it = keys.lowerBound(row);
		return it.value();
		*/
	}

	const TrackKey *getPrevKeyFrame(int row) const
	{
		assert(!"implement");
		/*
		QMap<int, TrackKey>::const_iterator it = keys.lowerBound(row);
		if (it != keys.constBegin() && (it == keys.constEnd() || it.key() != row))
			--it;

		if (it == keys.constEnd() || it.key() > row)
			return NULL;
		return &it.value();
			*/

		return NULL;
	}

	const TrackKey *getNextKeyFrame(int row) const
	{
		assert(!"implement");
		/*
		QMap<int, TrackKey>::const_iterator it = keys.lowerBound(row + 1);

		if (it == keys.constEnd() || it.key() < row)
			return NULL;

		return &it.value();
			*/

		return NULL;
	}

	static void getPolynomial(float coeffs[4], const TrackKey *key)
	{
		coeffs[0] = key->value;
		switch (key->type) {
		case TrackKey::STEP:
			coeffs[1] = coeffs[2] = coeffs[3] = 0.0f;
			break;

		case TrackKey::LINEAR:
			coeffs[1] = 1.0f;
			coeffs[2] = coeffs[3] = 0.0f;
			break;

		case TrackKey::SMOOTH:
			coeffs[1] = 0.0f;
			coeffs[2] = 3.0f;
			coeffs[3] = -2.0f;
			break;

		case TrackKey::RAMP:
			coeffs[1] = coeffs[3] = 0.0f;
			coeffs[2] = 1.0f;
			break;

		default:
			assert(0);
			coeffs[0] = 0.0f;
			coeffs[1] = 0.0f;
			coeffs[2] = 0.0f;
			coeffs[3] = 0.0f;
		}
	}

	double getValue(int row) const
	{
		if (!keys.size())
			return 0.0;

		const TrackKey *prevKey = getPrevKeyFrame(row);
		const TrackKey *nextKey = getNextKeyFrame(row);

		assert(prevKey != NULL || nextKey != NULL);
		assert(prevKey != nextKey);

		if (!prevKey)
			return nextKey->value;
		if (!nextKey)
			return prevKey->value;
		if (prevKey == nextKey)
			return prevKey->value;

		float coeffs[4];
		getPolynomial(coeffs, prevKey);

		float x = double(row - prevKey->row) /
			double(nextKey->row - prevKey->row);
		float mag = nextKey->value - prevKey->value;
		return coeffs[0] + (coeffs[1] + (coeffs[2] + coeffs[3] * x) * x) * x * mag;
	}

	const std::map<int, TrackKey> getKeyMap() const
	{
		return keys;
	}

	// Track is considered active if it has been requested by a client.
	bool isActive() const
	{
		return active;
	}

	void setActive(bool active)
	{
		this->active = active;
	}

	const std::string &getName() const { return name; }
	const std::string &getDisplayName() const { return displayName; }

private:
	std::string name, displayName;
	bool active;
	std::map<int, TrackKey> keys;

/*signals:
	void keyFrameAdded(int row);
	void keyFrameChanged(int row, const SyncTrack::TrackKey &old);
	void keyFrameRemoved(int row, const SyncTrack::TrackKey &old);
	*/
};

#endif // !defined(SYNCTRACK_H)
