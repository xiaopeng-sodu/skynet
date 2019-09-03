
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>


static int 
read_lua_array(unsigned char card[], int idx , lua_State *L, int maxn){
	int n = lua_rawlen(L, idx);

	if (n > maxn){
		return -1;
	}

	int i = 1;
	for(;i<n;i++){
		lua_rawgeti(L, idx, i);
		card[i -1] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
	return n;
}

static void 
sort_card_list(unsigned char data[], int count){
	int i; 
	for(i = 0;i<count; i++){
		int j;
		for(j = i+1;j<count;j++){
			if (data[j] < data[i]){
				int tmp = data[i];
				data[i] = data[j];
				data[j] = tmp;
			}
		}
	}
	return;
}

static int 
find_next(unsigned char data[], int count, int mid){
	if (mid < 0){
		return -1;
	}

	int i;
	for(i=0;i<count;i++){
		if (data[i] == mid){
			return i;
		}else if (data[i] > mid){
			return -1;
		}
	}

	return -1;
}

static int 
hupai(unsigned char data[], int count , int pair_num, int xz_num){
	if(pair_num > 1)
		return 0;

	if (xz_num < 0)
		return 0;

	for(;count > 0;){
		if (data[0] == 0){
			data ++;
			count --;
		}else{
			break;
		}
	}

	if (count <= 0){
		return 1;
	}

	int mid = data[0];
	data++;
	count--;

	int pos = find_next(data, count, mid); // find xx type
	if (pos != -1){
		data[pos] = 0;

		if (hupai(data, count, pair_num+1, xz_num)){
			return 1;
		}

		int pos1 = find_next(data + pos + 1, count - pos -1, mid) + pos + 1;  // find xxx type
		if (pos != pos1){
			data[pos1] = 0;
			if (hupai(data, count, pair_num, xz_num)){
				return 1;
			}
			data[pos1] = mid;
		}else{
			if (hupai(data, count, pair_num, xz_num - 1)){  // find xx + hongzhong
				return 1;
			}
		}

		data[pos] = mid;
	}else{
		if (hupai(data, count, pair_num +1, xz_num -1 )){  // find x + hongzhong
			return 1;
		}

		if (hupai(data, count, pair_num, xz_num -2)){  // find x+ 2hongzhong
			return 1;
		}
	}

	if (mid > 0x30){
		return 0;
	}

	pos = find_next(data, count, mid + 1);  // find xy
	if (pos != -1){
		data[pos] = 0;

		int pos1 = find_next(data, count - pos -1, mid+2) + pos + 1; // find xyz
		if (pos != pos1){
			data[pos1] = 0;
			if (hupai(data, count, pair_num, xz_num)){
				return 1;
			}
			data[pos1] = mid + 2;
		}else{
			if (hupai(data, count, pair_num, xz_num -1)){
				return 1;
			}
		}

		data[pos] = mid + 1;
	}

	pos = find_next(data, count, mid + 2) // find xz
	if (pos != -1){
		data[pos] = 0;

		if (hupai(data, count, pair_num, xz_num - 1)){
			return 1;
		}

		data[pos] = mid + 2;
	}

	return 0;
}

static int 
_is_hupai(unsigned char data[], int count, int xz_num, int HONG_ZHONG, int qidui, int pair_num){
	if ((count + xz_num + pair_num * 2 - 2) % 3 != 0){
		return 0;
	}

	int i; 
	for(;i<count;){
		if (data[i] == HONG_ZHONG){
			int tmp = data[i];
			data[i] = data[count - 1];
			data[count - 1] = tmp;

			xz_num = xz_num + 1;
			count--;
		}else{
			i++;
		}
	}

	if (xz_num >= 4){
		return 1;
	}

	sort_card_list(data, count);

	if ( (count + xz_num + pair_num * 2 == 14) && qidui){
		int i =0;
		int need_hz = 0;
		for(;i <count;){
			if (data[i]+1 != data[i+1]){
				if(need_hz > xz_num){
					break;
				}

				need_hz ++;
				i++;
			}else{
				i =i + 2;
			}
		}
		if (i >= count){
			return 1;
		}
	}

	return hupai()
}

static int 
is_hupai(lua_State *L){
	int hongzhong = lua_tonumber(L, 2);
	int qidui = lua_tonumber(L, 3);

	unsigned char card[14];
	int len = read_lua_array(card, 1, L, 14);
	if (len == -1){
		return 0;
	}
}

static int 
is_tingpai(lua_State *L){
	int hongzhong = lua_tonumber(L, 2);
	int qidui = lua_tonumber(L, 3);

	unsigned char card[14];
	int len = read_lua_array(L, 1, card, 13);

	if (len == -1){
		return 0;
	}

	int hupai = _is_hupai(card, len, 1, hongzhong, qidui, 0);
	if (hupai){
		lua_pushnumber(L, hupai);
		return 1;
	}else{
		return 0;
	}
}

static int 
luaopen_majiang(lua_State *L ){
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"is_hupai", is_hupai},
		{NULL, NULL}
	}

	luaL_newlib(L, l);
	return 1;
}