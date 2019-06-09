#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <cstdio>
namespace sjtu {
	const char BPTREE_ADDRESS[128] = "bpt";
	template <class Key, class Value, class Compare = std::less<Key> >
	class BTree {
	private:
		class kuaixinxi {
		public:
			bool blockT = false;
			int bsize = 0;
			int bpos = 0;
			int bpar = 0;
			int bpre = 0;
			int bnext = 0;
		};

		struct DataNode {
			int nodeerzi = 0;
			Key nodekey;
		};

		const static int kuaisize = 4096;
		const static int kuaixinxisize = sizeof(kuaixinxi);
		const static int keysize = sizeof(Key);
		const static int valuesize = sizeof(Value);
		const static int kgeshu = (kuaisize - kuaixinxisize) / sizeof(DataNode) - 1;
		const static int pgeshu = (kuaisize - kuaixinxisize) / (keysize + valuesize) - 1;

		class wjxinxi {
		public:
			int geshu = 1;
			introotpos = 0;
			int data_blockHead = 0;
			int data_block_rear = 0;
			int bsize = 0;
		};

		class wjk {
		public:
			DataNode val[kgeshu];
		};

		class yedata {
		public:
			pair<Key, Value> val[pgeshu];
		};

		wjxinxi bpshux;

		static FILE* wenjian;

		template <class T>
		static void memdu(T buff, int buff_size, int pos) {
			fseek(wenjian, long(buff_size * pos), SEEK_SET);
			fread(buff, buff_size, 1, wenjian);
		}

		template <class T>
		static void memxie(T buff, int buff_size, int pos) {
			fseek(wenjian, long(buff_size * pos), SEEK_SET);
			fwrite(buff, buff_size, 1, wenjian);
			fflush(wenjian);
		}

		void write_contentOfTree() {
			fseek(wenjian, 0, SEEK_SET);
			char buff[kuaisize] = { 0 };
			memcpy(buff, &bpshux, sizeof(bpshux));
			memxie(buff, kuaisize, 0);
		}

		int memory_get() {
			++bpshux.geshu;
			write_contentOfTree();
			char buff[kuaisize] = { 0 };
			memxie(buff, kuaisize, bpshux.geshu - 1);
			return bpshux.geshu - 1;
		}

		int jiannode(int bpar) {
			auto node_pos = memory_get();
			kuaixinxi temp;
			wjk normalData;
			temp.blockT = false;
			temp.bpar = bpar;
			temp.bpos = node_pos;
			temp.bsize = 0;
			xienode(&temp, &normalData, node_pos);
			return node_pos;
		}

		int jianye(int bpar, int bpre, int bnext) {
			auto node_pos = memory_get();
			kuaixinxi temp;
			yedata leafData;
			temp.blockT = true;
			temp.bpar = bpar;
			temp.bpos = node_pos;
			temp.bpre = bpre;
			temp.bnext = bnext;
			temp.bsize = 0;
			xienode(&temp, &leafData, node_pos);
			return node_pos;
		}

		void charubz(kuaixinxi & parent_info, wjk & parent_data,
			int origin, int new_pos, const Key & new_index) {
			++parent_info.bsize;
			auto p = parent_info.bsize - 2;
			for (; parent_data.val[p].nodeerzi != origin; --p) {
				parent_data.val[p + 1] = parent_data.val[p];
			}
			parent_data.val[p + 1].nodekey = parent_data.val[p].nodekey;
			parent_data.val[p].nodekey = new_index;
			parent_data.val[p + 1].nodeerzi = new_pos;
		}

		template <class DATA_TYPE>
		static void xienode(kuaixinxi * _info, DATA_TYPE * _data, int bpos) {
			char buff[kuaisize] = { 0 };
			memcpy(buff, _info, sizeof(kuaixinxi));
			memcpy(buff + kuaixinxisize, _data, sizeof(DATA_TYPE));
			memxie(buff, kuaisize, bpos);
		}

		template <class DATA_TYPE>
		static void dunode(kuaixinxi * _info, DATA_TYPE * _data, int bpos) {
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, bpos);
			memcpy(_info, buff, sizeof(kuaixinxi));
			memcpy(_data, buff + kuaixinxisize, sizeof(DATA_TYPE));
		}

		Key fenyezi(int pos, kuaixinxi & origin_info, yedata & origin_data) {
			int parent_pos;
			kuaixinxi parent_info;
			wjk parent_data;

			if (pos == bpshux.root_pos) {
				autorootpos = jiannode(0);
				bpshux.root_pos =rootpos;
				write_contentOfTree();
				dunode(&parent_info, &parent_data,rootpos);
				origin_info.bpar =rootpos;
				++parent_info.bsize;
				parent_data.val[0].nodeerzi = pos;
				parent_pos =rootpos;
			}
			else {
				dunode(&parent_info, &parent_data, origin_info.bpar);
				parent_pos = parent_info.bpos;
			}
			if (jcpar(origin_info)) {
				parent_pos = origin_info.bpar;
				dunode(&parent_info, &parent_data, parent_pos);
			}
			auto new_pos = jianye(parent_pos, pos, origin_info.bnext);

			auto temp_pos = origin_info.bnext;
			kuaixinxi temp_info;
			yedata temp_data;
			dunode(&temp_info, &temp_data, temp_pos);
			temp_info.bpre = new_pos;
			xienode(&temp_info, &temp_data, temp_pos);
			origin_info.bnext = new_pos;

			kuaixinxi new_info;
			yedata new_data;
			dunode(&new_info, &new_data, new_pos);

			int mid_pos = origin_info.bsize >> 1;
			for (int p = mid_pos, i = 0; p < origin_info.bsize; ++p, ++i) {
				new_data.val[i].first = origin_data.val[p].first;
				new_data.val[i].second = origin_data.val[p].second;
				++new_info.bsize;
			}
			origin_info.bsize = mid_pos;
			charubz(parent_info, parent_data, pos, new_pos, origin_data.val[mid_pos].first);

			xienode(&origin_info, &origin_data, pos);
			xienode(&new_info, &new_data, new_pos);
			xienode(&parent_info, &parent_data, parent_pos);

			return new_data.val[0].first;
		}

		bool jcpar(kuaixinxi & child_info) {
			int parent_pos, origin_pos = child_info.bpar;
			kuaixinxi parent_info, origin_info;
			wjk parent_data, origin_data;
			dunode(&origin_info, &origin_data, origin_pos);
			if (origin_info.bsize < kgeshu)
				return false;

			if (origin_pos == bpshux.root_pos) {
				autorootpos = jiannode(0);
				bpshux.root_pos =rootpos;
				write_contentOfTree();
				dunode(&parent_info, &parent_data,rootpos);
				origin_info.bpar =rootpos;
				++parent_info.bsize;
				parent_data.val[0].nodeerzi = origin_pos;
				parent_pos =rootpos;
			}
			else {
				dunode(&parent_info, &parent_data, origin_info.bpar);
				parent_pos = parent_info.bpos;
			}
			if (jcpar(origin_info)) {
				parent_pos = origin_info.bpar;
				dunode(&parent_info, &parent_data, parent_pos);
			}
			auto new_pos = jiannode(parent_pos);
			kuaixinxi new_info;
			wjk new_data;
			dunode(&new_info, &new_data, new_pos);

			int mid_pos = origin_info.bsize >> 1;
			for (int p = mid_pos + 1, i = 0; p < origin_info.bsize; ++p, ++i) {
				if (origin_data.val[p].nodeerzi == child_info.bpos) {
					child_info.bpar = new_pos;
				}
				std::swap(new_data.val[i], origin_data.val[p]);
				++new_info.bsize;
			}
			origin_info.bsize = mid_pos + 1;
			charubz(parent_info, parent_data, origin_pos, new_pos, origin_data.val[mid_pos].nodekey);

			xienode(&origin_info, &origin_data, origin_pos);
			xienode(&new_info, &new_data, new_pos);
			xienode(&parent_info, &parent_data, parent_pos);
			return true;
		}

		void cbz(int l_parent, int l_child, const Key & new_key) {
			kuaixinxi parent_info;
			wjk parent_data;
			dunode(&parent_info, &parent_data, l_parent);
			if (parent_data.val[parent_info.bsize - 1].nodeerzi == l_child) {
				cbz(parent_info.bpar, l_parent, new_key);
				return;
			}
			for (int cur_pos = parent_info.bsize - 2;; --cur_pos) {
				if (parent_data.val[cur_pos].nodeerzi == l_child) {
					parent_data.val[cur_pos].nodekey = new_key;
					break;
				}
			}
			xienode(&parent_info, &parent_data, l_parent);
		}

		void hebing(kuaixinxi & l_info, wjk & l_data, kuaixinxi & r_info, wjk & r_data) {
			for (int p = l_info.bsize, i = 0; i < r_info.bsize; ++p, ++i) {
				l_data.val[p] = r_data.val[i];
			}
			l_data.val[l_info.bsize - 1].nodekey = adjust(r_info.bpar, r_info.bpos);
			l_info.bsize += r_info.bsize;
			xienode(&l_info, &l_data, l_info.bpos);
		}

		void pinghengn(kuaixinxi & info, wjk & normalData) {
			if (info.bsize >= kgeshu / 2) {
				xienode(&info, &normalData, info.bpos);
				return;
			}
			if (info.bpos == bpshux.root_pos && info.bsize <= 1) {
				bpshux.root_pos = normalData.val[0].nodeerzi;
				write_contentOfTree();
				return;
			}
			else if (info.bpos == bpshux.root_pos) {
				xienode(&info, &normalData, info.bpos);
				return;
			}

			kuaixinxi parent_info, brother_info;
			wjk parent_data, brother_data;
			dunode(&parent_info, &parent_data, info.bpar);
			int value_pos;
			for (value_pos = 0; parent_data.val[value_pos].nodeerzi != info.bpos; ++value_pos);
			if (value_pos > 0) {
				dunode(&brother_info, &brother_data, parent_data.val[value_pos - 1].nodeerzi);
				brother_info.bpar = info.bpar;
				if (brother_info.bsize > kgeshu / 2) {
					for (int p = info.bsize; p > 0; --p) {
						normalData.val[p] = normalData.val[p - 1];
					}
					normalData.val[0].nodeerzi = brother_data.val[brother_info.bsize - 1].nodeerzi;
					normalData.val[0].nodekey = parent_data.val[value_pos - 1].nodekey;
					parent_data.val[value_pos - 1].nodekey = brother_data.val[brother_info.bsize - 2].nodekey;
					--brother_info.bsize;
					++info.bsize;
					xienode(&brother_info, &brother_data, brother_info.bpos);
					xienode(&info, &normalData, info.bpos);
					xienode(&parent_info, &parent_data, parent_info.bpos);
					return;
				}
				else {
					hebing(brother_info, brother_data, info, normalData);
					return;
				}
			}
			if (value_pos < parent_info.bsize - 1) {
				dunode(&brother_info, &brother_data, parent_data.val[value_pos + 1].nodeerzi);
				brother_info.bpar = info.bpar;
				if (brother_info.bsize > kgeshu / 2) {
					normalData.val[info.bsize].nodeerzi = brother_data.val[0].nodeerzi;
					normalData.val[info.bsize - 1].nodekey = parent_data.val[value_pos].nodekey;
					parent_data.val[value_pos].nodekey = brother_data.val[0].nodekey;
					for (int p = 1; p < brother_info.bsize; ++p) {
						brother_data.val[p - 1] = brother_data.val[p];
					}
					--brother_info.bsize;
					++info.bsize;
					xienode(&brother_info, &brother_data, brother_info.bpos);
					xienode(&info, &normalData, info.bpos);
					xienode(&parent_info, &parent_data, parent_info.bpos);
					return;
				}
				else {
					hebing(info, normalData, brother_info, brother_data);
					return;
				}
			}
		}

		Key adjust(int pos, int removed_child) {
			kuaixinxi info;
			wjk normalData;
			dunode(&info, &normalData, pos);
			int cur_pos;
			for (cur_pos = 0; normalData.val[cur_pos].nodeerzi != removed_child; ++cur_pos);
			Key ans = normalData.val[cur_pos - 1].nodekey;
			normalData.val[cur_pos - 1].nodekey = normalData.val[cur_pos].nodekey;
			for (; cur_pos < info.bsize - 1; ++cur_pos) {
				normalData.val[cur_pos] = normalData.val[cur_pos + 1];
			}
			--info.bsize;
			pinghengn(info, normalData);
			return ans;
		}

		void hebingye(kuaixinxi & l_info, yedata & l_data, kuaixinxi & r_info, yedata & r_data) {
			for (int p = l_info.bsize, i = 0; i < r_info.bsize; ++p, ++i) {
				l_data.val[p].first = r_data.val[i].first;
				l_data.val[p].second = r_data.val[i].second;
			}
			l_info.bsize += r_info.bsize;
			adjust(r_info.bpar, r_info.bpos);
			l_info.bnext = r_info.bnext;
			kuaixinxi temp_info;
			yedata temp_data;
			dunode(&temp_info, &temp_data, r_info.bnext);
			temp_info.bpre = l_info.bpos;
			xienode(&temp_info, &temp_data, temp_info.bpos);
			xienode(&l_info, &l_data, l_info.bpos);
		}

		void balance_leaf(kuaixinxi & leaf_info, yedata & leafData) {
			if (leaf_info.bsize >= pgeshu / 2) {
				xienode(&leaf_info, &leafData, leaf_info.bpos);
				return;
			}
			else if (leaf_info.bpos == bpshux.root_pos) {
				if (leaf_info.bsize == 0) {
					kuaixinxi temp_info;
					yedata temp_data;
					dunode(&temp_info, &temp_data, bpshux.data_blockHead);
					temp_info.bnext = bpshux.data_block_rear;
					xienode(&temp_info, &temp_data, bpshux.data_blockHead);
					dunode(&temp_info, &temp_data, bpshux.data_block_rear);
					temp_info.bpre = bpshux.data_blockHead;
					xienode(&temp_info, &temp_data, bpshux.data_block_rear);
					return;
				}
				xienode(&leaf_info, &leafData, leaf_info.bpos);
				return;
			}

			kuaixinxi brother_info, parent_info;
			yedata brother_data;
			wjk parent_data;

			dunode(&parent_info, &parent_data, leaf_info.bpar);
			int node_pos = 0;
			for (; node_pos < parent_info.bsize; ++node_pos) {
				if (parent_data.val[node_pos].nodeerzi == leaf_info.bpos)
					break;
			}

			if (node_pos > 0) {
				dunode(&brother_info, &brother_data, leaf_info.bpre);
				brother_info.bpar = leaf_info.bpar;
				if (brother_info.bsize > pgeshu / 2) {
					for (int p = leaf_info.bsize; p > 0; --p) {
						leafData.val[p].first = leafData.val[p - 1].first;
						leafData.val[p].second = leafData.val[p - 1].second;
					}
					leafData.val[0].first = brother_data.val[brother_info.bsize - 1].first;
					leafData.val[0].second = brother_data.val[brother_info.bsize - 1].second;
					--brother_info.bsize;
					++leaf_info.bsize;
					cbz(brother_info.bpar, brother_info.bpos, leafData.val[0].first);
					xienode(&brother_info, &brother_data, brother_info.bpos);
					xienode(&leaf_info, &leafData, leaf_info.bpos);
					return;
				}
				else {
					hebingye(brother_info, brother_data, leaf_info, leafData);
					return;
				}
			}

			if (node_pos < parent_info.bsize - 1) {
				dunode(&brother_info, &brother_data, leaf_info.bnext);
				brother_info.bpar = leaf_info.bpar;
				if (brother_info.bsize > pgeshu / 2) {
					leafData.val[leaf_info.bsize].first = brother_data.val[0].first;
					leafData.val[leaf_info.bsize].second = brother_data.val[0].second;
					for (int p = 1; p < brother_info.bsize; ++p) {
						brother_data.val[p - 1].first = brother_data.val[p].first;
						brother_data.val[p - 1].second = brother_data.val[p].second;
					}
					++leaf_info.bsize;
					--brother_info.bsize;
					cbz(leaf_info.bpar, leaf_info.bpos, brother_data.val[0].first);
					xienode(&leaf_info, &leafData, leaf_info.bpos);
					xienode(&brother_info, &brother_data, brother_info.bpos);
					return;
				}
				else {
					hebingye(leaf_info, leafData, brother_info, brother_data);
					return;
				}
			}
		}

		void jcfile() {
			if (!wenjian) {
				wenjian = fopen(BPTREE_ADDRESS, "wb+");
				write_contentOfTree();

				auto node_head = bpshux.geshu,
					node_rear = bpshux.geshu + 1;

				bpshux.data_blockHead = node_head;
				bpshux.data_block_rear = node_rear;

				jianye(0, 0, node_rear);
				jianye(0, node_head, 0);

				return;
			}
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, 0);
			memcpy(&bpshux, buff, sizeof(bpshux));
		}
	public:
		typedef pair<const Key, Value> value_type;

		class const_iterator;
		class iterator {
			friend class sjtu::BTree<Key, Value, Compare>::const_iterator;
			friend iterator sjtu::BTree<Key, Value, Compare>::begin();
			friend iterator sjtu::BTree<Key, Value, Compare>::end();
			friend iterator sjtu::BTree<Key, Value, Compare>::find(const Key&);
			friend pair<iterator, OperationResult> sjtu::BTree<Key, Value, Compare>::insert(const Key&, const Value&);
		private:
			BTree* cur_bptree = nullptr;
			kuaixinxi block_info;
			int cur_pos = 0;

		public:
			bool modify(const Value& value) {
				kuaixinxi info;
				yedata leafData;
				dunode(&info, &leafData, block_info.bpos);
				leafData.val[cur_pos].second = value;
				xienode(&info, &leafData, block_info.bpos);
				return true;
			}
			iterator() {
			}
			iterator(const iterator& other) {
				cur_bptree = other.cur_bptree;
				block_info = other.block_info;
				cur_pos = other.cur_pos;
			}
			iterator operator++(int) {
				auto temp = *this;
				++cur_pos;
				if (cur_pos >= block_info.bsize) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bnext);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return temp;
			}
			iterator& operator++() {
				++cur_pos;
				if (cur_pos >= block_info.bsize) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bnext);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return *this;
			}
			iterator operator--(int) {
				auto temp = *this;
				if (cur_pos == 0) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bpre);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info.bsize - 1;
				}
				else
					--cur_pos;
				return temp;
			}
			iterator& operator--() {
				if (cur_pos == 0) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bpre);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info.bsize - 1;
				}
				else
					--cur_pos;

				return *this;
			}
			value_type operator*() const {
				if (cur_pos >= block_info.bsize)
					throw invalid_iterator();
				char buff[kuaisize] = { 0 };
				memdu(buff, kuaisize, block_info.bpos);
				yedata leafData;
				memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
				value_type result(leafData.val[cur_pos].first, leafData.val[cur_pos].second);
				return result;
			}
			bool operator==(const iterator & rhs) const {
				return cur_bptree == rhs.cur_bptree
					&& block_info.bpos == rhs.block_info.bpos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator==(const const_iterator & rhs) const {
				return block_info.bpos == rhs.block_info.bpos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator!=(const iterator & rhs) const {
				return cur_bptree != rhs.cur_bptree
					|| block_info.bpos != rhs.block_info.bpos
					|| cur_pos != rhs.cur_pos;
			}
			bool operator!=(const const_iterator & rhs) const {
				return block_info.bpos != rhs.block_info.bpos
					|| cur_pos != rhs.cur_pos;
			}
		};
		class const_iterator {
			friend class sjtu::BTree<Key, Value, Compare>::iterator;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::cbegin() const;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::cend() const;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::find(const Key&) const;
		private:
			kuaixinxi block_info;
			int cur_pos = 0;
		public:
			const_iterator() {
			}
			const_iterator(const const_iterator& other) {
			
			}
			const_iterator(const iterator& other) {
				
			}
			const_iterator operator++(int) {
				auto temp = *this;
				++cur_pos;
				if (cur_pos >= block_info.bsize) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bnext);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return temp;
			}
			const_iterator& operator++() {
				++cur_pos;
				if (cur_pos >= block_info.bsize) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bnext);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return *this;
			}
			const_iterator operator--(int) {
				auto temp = *this;
				if (cur_pos == 0) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bpre);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info.bsize - 1;
				}
				else
					--cur_pos;
				return temp;
			}
			const_iterator& operator--() {
				if (cur_pos == 0) {
					char buff[kuaisize] = { 0 };
					memdu(buff, kuaisize, block_info.bpre);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info.bsize - 1;
				}
				else
					--cur_pos;

				return *this;
			}
			value_type operator*() const {
				if (cur_pos >= block_info.bsize)
					throw invalid_iterator();
				char buff[kuaisize] = { 0 };
				memdu(buff, kuaisize, block_info.bpos);
				yedata leafData;
				memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
				value_type result(leafData.val[cur_pos].first, leafData.val[cur_pos].second);
				return result;
			}
			bool operator==(const iterator & rhs) const {
				return block_info.bpos == rhs.block_info.bpos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator==(const const_iterator & rhs) const {
				return block_info.bpos == rhs.block_info.bpos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator!=(const iterator & rhs) const {
				return block_info.bpos != rhs.block_info.bpos
					|| cur_pos != rhs.cur_pos;
			}
			bool operator!=(const const_iterator & rhs) const {
				return block_info.bpos != rhs.block_info.bpos
					|| cur_pos != rhs.cur_pos;
			}
		};
		BTree() {
			wenjian = fopen(BPTREE_ADDRESS, "rb+");
			if (!wenjian) {
				wenjian = fopen(BPTREE_ADDRESS, "wb+");
				write_contentOfTree();

				auto node_head = bpshux.geshu,
					node_rear = bpshux.geshu + 1;

				bpshux.data_blockHead = node_head;
				bpshux.data_block_rear = node_rear;

				jianye(0, 0, node_rear);
				jianye(0, node_head, 0);

				return;
			}
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, 0);
			memcpy(&bpshux, buff, sizeof(bpshux));
		}
		BTree(const BTree& other) {
			wenjian = fopen(BPTREE_ADDRESS, "rb+");
			bpshux.geshu = other.bpshux.geshu;
			bpshux.data_blockHead = other.bpshux.data_blockHead;
			bpshux.data_block_rear = other.bpshux.data_block_rear;
			bpshux.root_pos = other.bpshux.root_pos;
			bpshux.bsize = other.bpshux.bsize;
		}
		BTree& operator=(const BTree& other) {
			wenjian = fopen(BPTREE_ADDRESS, "rb+");
			bpshux.geshu = other.bpshux.geshu;
			bpshux.data_blockHead = other.bpshux.data_blockHead;
			bpshux.data_block_rear = other.bpshux.data_block_rear;
			bpshux.root_pos = other.bpshux.root_pos;
			bpshux.bsize = other.bpshux.bsize;
			return *this;
		}
		~BTree() {
			fclose(wenjian);
		}
		pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
			jcfile();
			if (empty()) {
				autorootpos = jianye(0, bpshux.data_blockHead, bpshux.data_block_rear);

				kuaixinxi temp_info;
				yedata temp_data;
				dunode(&temp_info, &temp_data, bpshux.data_blockHead);
				temp_info.bnext =rootpos;
				xienode(&temp_info, &temp_data, bpshux.data_blockHead);

				dunode(&temp_info, &temp_data, bpshux.data_block_rear);
				temp_info.bpre =rootpos;
				xienode(&temp_info, &temp_data, bpshux.data_block_rear);

				dunode(&temp_info, &temp_data,rootpos);
				++temp_info.bsize;
				temp_data.val[0].first = key;
				temp_data.val[0].second = value;
				xienode(&temp_info, &temp_data,rootpos);

				++bpshux.bsize;
				bpshux.root_pos =rootpos;
				write_contentOfTree();

				pair<iterator, OperationResult> result(begin(), Success);
				return result;
			}

			char buff[kuaisize] = { 0 };
			int cur_pos = bpshux.root_pos, cur_parent = 0;
			while (true) {
				memdu(buff, kuaisize, cur_pos);
				kuaixinxi temp;
				memcpy(&temp, buff, sizeof(temp));
				if (cur_parent != temp.bpar) {
					temp.bpar = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					memxie(buff, kuaisize, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				wjk normalData;
				memcpy(&normalData, buff + kuaixinxisize, sizeof(normalData));
				int child_pos = temp.bsize - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1].nodekey > key)) {
						break;
					}
				}
				cur_parent = cur_pos;
				cur_pos = normalData.val[child_pos].nodeerzi;
			}

			kuaixinxi info;
			memcpy(&info, buff, sizeof(info));
			yedata leafData;
			memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info.bsize && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					return pair<iterator, OperationResult>(end(), Fail);
				}
				if (value_pos >= info.bsize || leafData.val[value_pos].first > key) {
					if (info.bsize >= pgeshu) {
						auto cur_key = fenyezi(cur_pos, info, leafData);
						if (key > cur_key) {
							cur_pos = info.bnext;
							value_pos -= info.bsize;
							dunode(&info, &leafData, cur_pos);
						}
					}

					for (int p = info.bsize - 1; p >= value_pos; --p) {
						leafData.val[p + 1].first = leafData.val[p].first;
						leafData.val[p + 1].second = leafData.val[p].second;
						if (p == value_pos)
							break;
					}
					leafData.val[value_pos].first = key;
					leafData.val[value_pos].second = value;
					++info.bsize;
					xienode(&info, &leafData, cur_pos);
					iterator ans;
					ans.block_info = info;
					ans.cur_bptree = this;
					ans.cur_pos = value_pos;
					++bpshux.bsize;
					write_contentOfTree();
					pair<iterator, OperationResult> to_return(ans, Success);
					return to_return;
				}
			}
			return pair<iterator, OperationResult>(end(), Fail);
		}
		OperationResult erase(const Key & key) {
			jcfile();
			if (empty()) {
				return Fail;
			}
			char buff[kuaisize] = { 0 };
			int cur_pos = bpshux.root_pos, cur_parent = 0;
			while (true) {
				memdu(buff, kuaisize, cur_pos);
				kuaixinxi temp;
				memcpy(&temp, buff, sizeof(temp));
				if (cur_parent != temp.bpar) {
					temp.bpar = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					memxie(buff, kuaisize, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				wjk normalData;
				memcpy(&normalData, buff + kuaixinxisize, sizeof(normalData));
				int child_pos = temp.bsize - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1].nodekey > key)) {
						break;
					}
				}
				cur_parent = cur_pos;
				cur_pos = normalData.val[child_pos].nodeerzi;
			}

			kuaixinxi info;
			memcpy(&info, buff, sizeof(info));
			yedata leafData;
			memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info.bsize && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					--info.bsize;
					for (int p = value_pos; p < info.bsize; ++p) {
						leafData.val[p].first = leafData.val[p + 1].first;
						leafData.val[p].second = leafData.val[p + 1].second;
					}
					balance_leaf(info, leafData);
					--bpshux.bsize;
					write_contentOfTree();
					return Success;
				}
				if (value_pos >= info.bsize || leafData.val[value_pos].first > key) {
					return Fail;
				}
			}
			return Fail; 
		}
		iterator begin() {
			jcfile();
			iterator result;
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, bpshux.data_blockHead);
			kuaixinxi blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_bptree = this;
			result.cur_pos = 0;
			++result;
			return result;
		}
		const_iterator cbegin() const {
			const_iterator result;
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, bpshux.data_blockHead);
			kuaixinxi blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_pos = 0;
			++result;
			return result;
		}
		iterator end() {
			jcfile();
			iterator result;
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, bpshux.data_block_rear);
			kuaixinxi blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_bptree = this;
			result.cur_pos = 0;
			return result;
		}
		const_iterator cend() const {
			const_iterator result;
			char buff[kuaisize] = { 0 };
			memdu(buff, kuaisize, bpshux.data_block_rear);
			kuaixinxi blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_pos = 0;
			return result;
		}
		bool empty() const {
			if (!wenjian)
				return true;
			return bpshux.bsize == 0;
		}

		int size() const {
			if (!wenjian)
				return 0;
			return bpshux.bsize;
		}
		void clear() {
			if (!wenjian)
				return;
			remove(BPTREE_ADDRESS);
			wjxinxi new_fileHead;
			bpshux = new_fileHead;
			wenjian = nullptr;
		}
		Value at(const Key & key) {
			if (empty()) {
				throw container_is_empty();
			}

			char buff[kuaisize] = { 0 };
			int cur_pos = bpshux.root_pos, cur_parent = 0;
			while (true) {
				memdu(buff, kuaisize, cur_pos);
				kuaixinxi temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp.bpar) {
					temp.bpar = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					memxie(buff, kuaisize, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				wjk normalData;
				memcpy(&normalData, buff + kuaixinxisize, sizeof(normalData));
				int child_pos = temp.bsize - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1].nodekey > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos].nodeerzi;
			}
			kuaixinxi info;
			memcpy(&info, buff, sizeof(info));
			yedata leafData;
			memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info.bsize && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					return leafData.val[value_pos].second;
				}
				if (value_pos >= info.bsize || leafData.val[value_pos].first > key) {
					throw index_out_of_bound();
				}
			}
		}
		int count(const Key & key) const {
			return find(key) == cend() ? 0 : 1;
		}
		iterator find(const Key & key) {
			if (empty()) {
				return end();
			}

			char buff[kuaisize] = { 0 };
			int cur_pos = bpshux.root_pos, cur_parent = 0;
			while (true) {
				memdu(buff, kuaisize, cur_pos);
				kuaixinxi temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp.bpar) {
					temp.bpar = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					memxie(buff, kuaisize, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				wjk normalData;
				memcpy(&normalData, buff + kuaixinxisize, sizeof(normalData));
				int child_pos = temp.bsize - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1].nodekey > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos].nodeerzi;
			}
			kuaixinxi info;
			memcpy(&info, buff, sizeof(info));
			sizeof(wjk);
			yedata leafData;
			memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info.bsize && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					iterator result;
					result.cur_bptree = this;
					result.block_info = info;
					result.cur_pos = value_pos;
					return result;
				}
				if (value_pos >= info.bsize || leafData.val[value_pos].first > key) {
					return end();
				}
			}
			return end();
		}
		const_iterator find(const Key & key) const {
			if (empty()) {
				return cend();
			}

			char buff[kuaisize] = { 0 };
			int cur_pos = bpshux.root_pos, cur_parent = 0;
			while (true) {
				memdu(buff, kuaisize, cur_pos);
				kuaixinxi temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp.bpar) {
					temp.bpar = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					memxie(buff, kuaisize, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				wjk normalData;
				memcpy(&normalData, buff + kuaixinxisize, sizeof(normalData));
				int child_pos = temp.bsize - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1].nodekey > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos].nodeerzi;
			}
			kuaixinxi info;
			memcpy(&info, buff, sizeof(info));
			yedata leafData;
			memcpy(&leafData, buff + kuaixinxisize, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info.bsize && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					const_iterator result;
					result.block_info = info;
					result.cur_pos = value_pos;
					return result;
				}
				if (value_pos >= info.bsize || leafData.val[value_pos].first > key) {
					return cend();
				}
			}
			return cend();
		}
	};
	template <typename Key, typename Value, typename Compare> FILE* BTree<Key, Value, Compare>::wenjian = nullptr;
}
