#include <cstdio>
#include <cstring>
#include <iostream>

using std::cout;
using std::endl;

#define NAME_SIZE 20
#define EMPTY_PTR -1

struct Header {
	int activeCount;
	int deletedCount;
	int firstActive;
	int firstDeleted;
	int lastDeleted;
};

struct Record {
	unsigned char deleted;
	char name[NAME_SIZE];
	int next;
};

// Вычисление смещения для record по индексу
int record_offset(int index) {
	return sizeof(Header) + index * sizeof(Record);
}

// Чтение header из файла
bool read_header(FILE* file, Header& header) {
	if (std::fseek(file, 0, SEEK_SET) != 0) {
		return false;
	}
	return std::fread(&header, sizeof(Header), 1, file) == 1;
}

// Запись header в файл
bool write_header(FILE* file, const Header& header) {
	if (std::fseek(file, 0, SEEK_SET) != 0) {
		return false;
	}
	return std::fwrite(&header, sizeof(Header), 1, file) == 1;
}

// Чтение record по смещению
bool read_record(FILE* file, int offset, Record& record) {
	if (std::fseek(file, offset, SEEK_SET) != 0) {
		return false;
	}
	return std::fread(&record, sizeof(Record), 1, file) == 1;
}

// Запись record по смещению
bool write_record(FILE* file, int offset, const Record& record) {
	if (std::fseek(file, offset, SEEK_SET) != 0) {
		return false;
	}
	return std::fwrite(&record, sizeof(Record), 1, file) == 1;
}

// Функция для сравнения имён
bool names_cmp(const char* lhs, const char* rhs) {
	return std::strncmp(lhs, rhs, NAME_SIZE) == 0;
}

// Функция для удаления записи по имени.
// 1 - удаление успешно, 0 - имя не найдено, -1 - ошибка
int remove(const char* filename, const char* name) {
	FILE* file = std::fopen(filename, "rb+");
	if (!file) {
		return -1;
	}

	Header header{};
	if (!read_header(file, header)) {
		std::fclose(file);
		return -1;
	}

	// Проходим по списку активных записей, чтобы найти запись с заданным именем
	int prevOffset = EMPTY_PTR;
	int curOffset = header.firstActive;

	while (curOffset != EMPTY_PTR) {
		Record cur{};
		if (!read_record(file, curOffset, cur)) {
			std::fclose(file);
			return -1;
		}

		// Если запись не удалена и имя совпало, удаляем её
		if (cur.deleted == 0 && names_cmp(cur.name, name)) {
			if (prevOffset == EMPTY_PTR) {
				header.firstActive = cur.next;
			} else {
				// Если удаляемая запись не первая, то нужно обновить next предыдущей записи
				Record prev{};
				if (!read_record(file, prevOffset, prev)) {
					std::fclose(file);
					return -1;
				}
				prev.next = cur.next;
				if (!write_record(file, prevOffset, prev)) {
					std::fclose(file);
					return -1;
				}
			}

			// Помечаем текущую запись как удалённую и добавляем её в очередь удалённых
			cur.deleted = 1;
			cur.next = EMPTY_PTR;
			if (!write_record(file, curOffset, cur)) {
				std::fclose(file);
				return -1;
			}

			// Если это первая удалённая запись, то она становится
			// первой и последней в очереди удалённых
			if (header.deletedCount == 0) {
				header.firstDeleted = curOffset;
				header.lastDeleted = curOffset;
			} else {
				// Иначе добавляем её в конец очереди удалённых,
				// обновляя указатель на последнюю удалённую запись
				Record lastDeleted{};
				if (!read_record(file, header.lastDeleted, lastDeleted)) {
					std::fclose(file);
					return -1;
				}
				lastDeleted.next = curOffset;
				if (!write_record(file, header.lastDeleted, lastDeleted)) {
					std::fclose(file);
					return -1;
				}
				header.lastDeleted = curOffset;
			}

			header.activeCount--;
			header.deletedCount++;

			if (!write_header(file, header)) {
				std::fclose(file);
				return -1;
			}

			std::fflush(file);
			std::fclose(file);
			return 1;
		}

		prevOffset = curOffset;
		curOffset = cur.next;
	}

	std::fclose(file);
	return 0;
}

// Вывод списка записей, начиная с заданного смещения
void print_list(FILE* file, int firstOffset) {
	int ptr = firstOffset;
	while (ptr != EMPTY_PTR) {
		Record r{};
		if (!read_record(file, ptr, r)) {
			cout << "Broken pointer " << ptr << endl;
			return;
		}
		cout << "[" << r.name << ", off=" << ptr << ", del=" << static_cast<int>(r.deleted)
			 << ", next=" << r.next << "]\n";
		ptr = r.next;
	}
}

// Функция для печати текущего состояния файла: заголовок, активные записи и удалённые запис
void print_state(const char* filename) {
	FILE* file = std::fopen(filename, "rb");
	if (!file) {
		cout << "Failed to open file: " << filename << endl;
		return;
	}

	Header h{};
	if (!read_header(file, h)) {
		cout << "Failed to read header" << endl;
		std::fclose(file);
		return;
	}

	cout << "Header: active=" << h.activeCount
		 << ", deleted=" << h.deletedCount
		 << ", firstActive=" << h.firstActive
		 << ", firstDeleted=" << h.firstDeleted
		 << ", lastDeleted=" << h.lastDeleted << endl;

	cout << "Active list:\n";
	print_list(file, h.firstActive);
	cout << endl;

	cout << "Deleted queue:" << endl;
	print_list(file, h.firstDeleted);
	cout << endl;
	std::fclose(file);
}


int main(int argc, char* argv[]) {
	if (argc < 3) {
		cout << "Invalid number of arguments. Provide a filename and at least one name to delete." << endl;
		return 1;
	}

	const char* filename = argv[1];

	cout << "State before deletion:" << endl;
	print_state(filename);

	bool errs = false;

	for (int i = 2; i < argc; ++i) {
		const char* nameToRemove = argv[i];
		const int result = remove(filename, nameToRemove);

		if (result == 1) {
			cout << "Deleted name: " << nameToRemove << endl;
		} else if (result == 0) {
			cout << "Name not found: " << nameToRemove << endl;
		} else {
			cout << "Error deleting name: " << nameToRemove << endl;
			errs = true;
		}
	}

	cout << "\nState after deletion:" << endl;
	print_state(filename);

	return errs ? 1 : 0;
}

