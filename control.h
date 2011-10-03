struct control_key {
	int kup;
	int kdown;
	float *value;
	float d;
	float min, max;
	int flags;
};

#define MAX_KEYS	30

struct control_data {
	float *origin;
	float *yaw;
	float *pitch;
	float *roll;
	struct control_key keys[MAX_KEYS];
	int nkeys;
	void *data;
	void *view;
	void *zbuffer;
};
#define WIDTH	640
#define HEIGHT	480
