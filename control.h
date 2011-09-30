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
	float *vup;
	float *vright;
	float *vpn;
	struct control_key keys[MAX_KEYS];
	int nkeys;
	void *data;
};
#define WIDTH	640
#define HEIGHT	480
