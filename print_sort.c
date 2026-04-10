#include <stdio.h>
#include <string.h>
#include <locale.h>
struct Header {
    int activeCount;      // количество активных элементов
    int deletedCount;     // количество удалённых
    int firstActive;      // указатель на первый активный элемент
    int firstDeleted;     // указатель на первый элемент в очереди удалённых
    int lastDeleted;      // указатель на последний элемент в очереди удалённых
};

struct Element {
    char deletedFlag;     // бит удаления (1 - удалён, 0 - активен)
    char name[20];        // наименование (строка, 20 байт)
    int next;             // указатель на следующий элемент
};



void showHelp(char* programName) {
    printf("Help: \n");
    printf("Sorted list printer\n");
    printf("Use:\n");
    printf("  %s <file_name>\n\n", programName);
    printf("Arguments:\n");
    printf("  <file_name> \n\n");
}

int main(int argc, char* argv[]) {
    
    setlocale(LC_ALL, "");
    if (argc != 2) {
        printf("Not enough arguments! (pro tip: try inputting file name too!)\n");
        showHelp(argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "rb");
    if (file == NULL) {
        printf("Couldn't open file\n");
        return 1;
    }
    
    struct Header header;
   
    fread(&header, sizeof(header), 1, file);
    int pos = header.firstActive; 
    int count = 0;
 

    char names[100][20];  

    
    if (header.activeCount == 0) {
        printf("It's empty!\n");
        fclose(file);
        return 0;
    }
    
    while (pos != -1 && count < 100) {
    // Ищем запись по смещению pos
        if (fseek(file, pos, SEEK_SET) != 0) {
            printf("Ошибка: неверный указатель %d\n", pos);
            break;
        }
        
        struct Element element;
        if (fread(&element, sizeof(element), 1, file) != 1) {
            printf("Ошибка чтения записи\n");
            break;
        }
        
        if (element.deletedFlag == 0) {
            strcpy(names[count], element.name);
            count++;
        }
        
        pos = element.next; 
    }
    
    // Сортировка
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(names[i], names[j]) > 0) {
                char temp[20];
                strcpy(temp, names[i]);
                strcpy(names[i], names[j]);
                strcpy(names[j], temp);
            }
        }
    }
    
    // Вывод
    printf("\nList:\n");
    for (int i = 0; i < count; i++) {
        printf("%d. %s\n", i + 1, names[i]);
    }
    printf("Overall: %d\n", count);

    
    fclose(file);
    
    return 0;
}
