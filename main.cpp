#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <fstream>
using namespace std;

SOCKET Connection;

int validationInput();
bool makeChoice(char& choise);
bool makeChoice_in_order(char& choice);

enum Request_Codes
{
    AUTHORIZATION = 11111,
    CHECK_BALANCE = 22222,
    ITEM_DETAILED_INFO = 33333,
    SELL_MENU = 44444,
    REGISTRATION = 55555
};

void show_item_detail_info()
{
    cout<<"===showing item detailed info==="<<endl;

    string request_code=to_string(ITEM_DETAILED_INFO);
    int msg_size=sizeof(request_code);
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    cout<<"Enter item code: "<<endl;
    int i_c = validationInput();
    string str_item_code = to_string(i_c);
    msg_size=str_item_code.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, str_item_code.c_str(), msg_size, NULL);

    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    char* record_from_table = new char[msg_size + 1];
    record_from_table[msg_size] = '\0';
    recv(Connection, record_from_table, msg_size, NULL);

    cout<<record_from_table<<endl;

    delete[] record_from_table;

}

void show_inventory_balance()
{
    cout<<"===showing inventory total balance==="<<endl;

    string request_code=to_string(CHECK_BALANCE);
    int msg_size=request_code.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    cout<<"msg_size "<<msg_size<<endl;
    char* records_from_table = new char[msg_size + 1];
    records_from_table[msg_size] = '\0';
    recv(Connection, records_from_table, msg_size, NULL);
    cout<<"id  name    amount  price"<<endl;
    cout<<records_from_table<<endl;

    delete[] records_from_table;

}

void sell_goods()
{
    cout<<"===selling goods==="<<endl;
    stringstream ss_order;//продумать работу с sstream
    char choice;
    string temp_str;

    string request_code=to_string(SELL_MENU);
    int msg_size=sizeof(request_code);
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    //цикл набора заказа
    do
    {
    cout<<"Enter item code: ";
    int it_code = validationInput();
    string str_item_code = to_string(it_code);
    msg_size=str_item_code.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, str_item_code.c_str(), msg_size, NULL);

    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    char* records_from_table = new char[msg_size + 1];
    records_from_table[msg_size] = '\0';
    recv(Connection, records_from_table, msg_size, NULL);

    //сделать тут вектор строк или multimap как на сервере; использ _ как разделитель, * как множитель заказ*количество
    temp_str = records_from_table;

    delete[] records_from_table;

    string rec1 = temp_str.substr(0, temp_str.find('*'));
    temp_str.erase(0, temp_str.find('*')+1);

    string rec2 = temp_str.substr(0, temp_str.find('*'));
    temp_str.erase(0, temp_str.find('*')+1);

    string rec3 = temp_str.substr(0, temp_str.find('*'));
    temp_str.erase(0, temp_str.find('*')+1);


    cout<<"In stock "<<rec2<<" "<<rec1<<" worth "<<rec3<<" rubles each."<<endl;
    cout<<"Enter quantity for the order: ";
    int amnt = validationInput();

    if(amnt>atoi(rec2.c_str()))//лучше бы сделать повторный запрос, вдруг др манагер обновит кол-во в другом потоке
    {
        cout<<"There is not enough product in stock for your order."<<endl;
        return;
    }

    ss_order<<str_item_code<<"*"<<to_string(amnt)<<"_";
    cout<<endl;
    cout<<"Product was added to your order"<<endl;

    temp_str=ss_order.str();
    cout<<"In your order:"<<endl;
    cout<<temp_str<<endl;

    }while(makeChoice_in_order(choice));// отправляет на сервер сообщение о продолжении ввода

    cout<<"Your final order: "<<endl;
    cout<<temp_str<<endl;

    //отправляем на сервер строку, содержащую заказ
    msg_size=temp_str.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, temp_str.c_str(), msg_size, NULL);

    //получаем общую стоимость всего заказа
    cout<<"Total cost of the order: ";
    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    char* order_total_cost = new char[msg_size + 1];
    order_total_cost[msg_size] = '\0';
    recv(Connection, order_total_cost, msg_size, NULL);
    cout<<order_total_cost<<endl;

    delete[] order_total_cost;

    //получаем имя и адрес клиента
    string str, customer_name, customer_address;

    bool gotName=false,
         gotAddress=false;

    cout<<"Confirm order? (y/n)";
    if(makeChoice(choice))
    {
        while(true)
        {
            if(gotName)
            {
                cout<<"Enter customer's address: ";
                getline(cin, str);
            }

            if(!gotName)
            {
                cout<<"Enter customer's name: "<<endl;
                getline(cin, str);
            }

            bool bRejected = false;

            for (unsigned int nIndex = 0; nIndex < str.length() && !bRejected; ++nIndex)
            {
                if (isalpha(str[nIndex]))
                    continue;
                if (isdigit(str[nIndex]))
                    continue;
                if (isspace(str[nIndex]))
                    continue;
                else
                {
                    cout<<"Invalid input"<<endl;
                    bRejected = true;
                }
            }

            if (gotName&&!bRejected)
            {
                gotAddress=true;
                customer_address=str;
                break;
            }

            if (!bRejected)
            {
                gotName=true;
                customer_name=str;
                bRejected=false;
            }

        }

        cout<<"Got name: "<<customer_name<<" and address: "<<customer_address<<endl;

        //отправляю имя*адрес клиента на сервер
        temp_str=customer_name+"*"+customer_address;
        msg_size=temp_str.size();
        send(Connection, (char*)&msg_size, sizeof(int), NULL);
        send(Connection, temp_str.c_str(), msg_size, NULL);
    }
    else
    {
        cout<<"deleting temporary data"<<endl;//отправляем запрос на удаление временного заказа
        temp_str="@denied@";
        msg_size=temp_str.size();
        send(Connection, (char*)&msg_size, sizeof(int), NULL);
        send(Connection, temp_str.c_str(), msg_size, NULL);
    }

        //принимаю данные всего заказа для формирования чека
        recv(Connection, (char*)&msg_size, sizeof(int), NULL);
        char* full_order_from_server = new char[msg_size + 1];
        full_order_from_server[msg_size] = '\0';
        recv(Connection, full_order_from_server, msg_size, NULL);
        cout<<full_order_from_server<<endl;

        //сохраняю заказ в txt

        time_t now = time(0);
        tm *gmtm = localtime(&now);
        string order_date = to_string(gmtm->tm_mday);
        order_date+='.';
        order_date.append(to_string(1+gmtm->tm_mon));

        string order = full_order_from_server;

        delete[] full_order_from_server;

        string customer_data = order.substr(0, order.find('#'));
        order.erase(0, order.find('#')+1);
        string goods_data = order;

        cout<<customer_data<<endl;
        cout<<goods_data<<endl;

        string order_name = "order_";
        order_name.append(customer_data.substr(0, customer_data.find('_')));
        order_name+='_';
        order_name+=order_date;
        order_name+=".txt";
        ofstream file(order_name);
        if(!file)
        {
            cerr<<"Error writing"<<endl;
            return;//заменить на exit/exception
        }

        file<<"\t\t\tOrder details"<<endl;
        file<<"Order number: ";
        file<<customer_data.substr(0, customer_data.find('_'))<<endl;
        customer_data.erase(0, customer_data.find('_')+1);

        file<<"Customer name: ";
        file<<customer_data.substr(0, customer_data.find('_'))<<endl;
        customer_data.erase(0, customer_data.find('_')+1);

        file<<"Customer address: ";
        file<<customer_data.substr(0, customer_data.find('_'))<<endl;
        customer_data.erase(0, customer_data.find('_')+1);

        file<<endl;
        file<<"Goods in order: "<<endl;

        while(!goods_data.empty())
        {
            file<<goods_data.substr(0, goods_data.find(','))<<" rubles"<<endl;
            goods_data.erase(0, goods_data.find(',')+1);
        }
        cout<<"Order completed!"<<endl;

        file.close();

}


void authorization(string& user_name, int& user_role)
{
    cout<<"Authorization..."<<endl;

    string str, str_login, str_pasw;

    bool gotLogin=false,
         gotPassword=false;

    while(true)
    {
        if(gotLogin)
        {
            cout<<"Enter password: "<<endl;
            getline(cin, str);
        }

        if(!gotLogin)
        {
            cout<<"Enter login: "<<endl;
            getline(cin, str);
        }

        bool bRejected = false;

        for (unsigned int nIndex = 0; nIndex < str.length() && !bRejected; ++nIndex)
        {
            if (isalpha(str[nIndex]))
                continue;
            if (isdigit(str[nIndex]))
                continue;
            if (str[nIndex] == '_') //(isspace(str[nIndex])) исправить? это пропускает 2 слова раздел пробелами при вводе
                continue;
            else
            {
                cout<<"Invalid input"<<endl;
                bRejected = true;
            }
        }

        if (gotLogin&&!bRejected)
        {
            gotPassword=true;
            str_pasw=str;
            break;
        }

        if (!bRejected)
        {
            gotLogin=true;
            str_login=str;
            bRejected=false;
        }

    }

    //сделать цикл на прием сообщений
    //названия переменных более понятные

    //отправляю код запроса, в if else на сервере обрабатывается
    string request_code=to_string(AUTHORIZATION);
    int msg_size=sizeof(request_code);
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    //отправляю логин*пароль
    string login_and_password = str_login+"*"+str_pasw;
    msg_size=login_and_password.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, login_and_password.c_str(), msg_size, NULL);

    //получаю роль пользователя; если 0, то меню в main не активируется
    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    char* user_role_from_bd = new char[msg_size + 1];
    user_role_from_bd[msg_size] = '\0';
    recv(Connection, user_role_from_bd, msg_size, NULL);
    user_role=atoi(user_role_from_bd);
    cout<<"user_role_from_bd: "<<user_role_from_bd<<endl;

    delete[] user_role_from_bd;

}

void registration()
{
    cout<<"===registration==="<<endl;

    string str, str_login, str_pasw, str_conf_pasw;

    bool gotLogin=false,
         gotPassword=false;

    while(true)
    {

        if(gotLogin&&gotPassword)
        {
            cout<<"Confirm password: "<<endl;
            getline(cin, str);
        }

        if(gotLogin&&!gotPassword)
        {
            cout<<"Enter password: "<<endl;
            getline(cin, str);
        }

        if(!gotLogin)
        {
            cout<<"Enter login: "<<endl;
            getline(cin, str);
        }

        bool bRejected = false;

        for (unsigned int nIndex = 0; nIndex < str.length() && !bRejected; ++nIndex)
        {
            if (isalpha(str[nIndex]))
                continue;
            if (isdigit(str[nIndex]))
                continue;
            if (str[nIndex] == '_') //(isspace(str[nIndex])) исправить? это пропускает 2 слова раздел пробелами при вводе
                continue;
            else
            {
                cout<<"Invalid input"<<endl;
                bRejected = true;
            }
        }

        if(gotLogin&&gotPassword&&!bRejected)
        {
            str_conf_pasw=str;
            break;
        }

        if (gotLogin&&!bRejected)
        {
            gotPassword=true;
            str_pasw=str;

        }

        if (!bRejected)
        {
            gotLogin=true;
            str_login=str;
            bRejected=false;
        }

    }

    if(str_pasw!=str_conf_pasw)
    {
        cout<<"Password isn't confirmed"<<endl;
        return;
    }

    string request_code=to_string(REGISTRATION);
    int msg_size=request_code.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    string name_and_passw = str_login+"*"+str_pasw;
    msg_size=name_and_passw.size();
    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, name_and_passw.c_str(), msg_size, NULL);


}


void make_order()
{
    cout<<"===making an order==="<<endl;
}

void check_order_status()//принимает инт номер заказа возвр 0 если нет заказа; иначе заказ в подробностях
{
    cout<<"===checking order status==="<<endl;
}

//возможно стоит сделать несколько вариантов с понятной подсказкой на повторение в cout<<continue что именно?
bool makeChoice(char& choice) // предлагаем повторить
{
	std::cout << "Do you want ";
	while (true)
    {
		std::cout << "to continue? yes/no - y/n : ";
		std::cin >> choice;
		std::cin.ignore(SHRT_MAX, '\n');
		if (choice == 'n' || choice == 'y') return (choice == 'y') ? true : false;
		else std::cout << "Error, do you want ";
	}
}

//удалить дубирующую ф-ю

bool makeChoice_in_order(char& choice) // предлагаем повторить
{
	int msg_size;
	string continue_order;
	std::cout << "Do you want ";
	while (true)
    {
		std::cout << "add another item? yes/no - y/n : ";
		std::cin >> choice;
		std::cin.ignore(SHRT_MAX, '\n');
		if (choice == 'y')
        {
            continue_order = "1";
            msg_size=continue_order.size();
            send(Connection, (char*)&msg_size, sizeof(int), NULL);
            send(Connection, continue_order.c_str(), msg_size, NULL);

            return true;
        }
        else if(choice == 'n')
        {
            continue_order = "0";
            msg_size=continue_order.size();
            send(Connection, (char*)&msg_size, sizeof(int), NULL);
            send(Connection, continue_order.c_str(), msg_size, NULL);

            return false;
        }
		else std::cout << "Input error, do you want ";
	}
}

bool makeChoice_in_server(char& choice) // предлагаем выбрать оформлять заказ или нет
{
	int msg_size;
	string continue_order;
	std::cout << "Do you want ";
	while (true)
    {
		std::cout << "place an order? yes/no - y/n : ";
		std::cin >> choice;
		std::cin.ignore(SHRT_MAX, '\n');
		if (choice == 'y')
        {
            continue_order = "1";
            msg_size=continue_order.size();
            send(Connection, (char*)&msg_size, sizeof(int), NULL);
            send(Connection, continue_order.c_str(), msg_size, NULL);

            return true;
        }
        else if(choice == 'n')
        {
            continue_order = "0";
            msg_size=continue_order.size();
            send(Connection, (char*)&msg_size, sizeof(int), NULL);
            send(Connection, continue_order.c_str(), msg_size, NULL);

            return false;
        }
		else std::cout << "Input error, do you want ";
	}
}


int validationInput()
{
    int val;
    while (true)
    {
        std::cin >> val;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(32767, '\n');
            std::cerr << "Input Error! Try again: ";
            continue;
        }
        std::cin.ignore(32767, '\n');
        return val;
    }
}

double validationInput(double)
{
    double val;
    while (true)
    {
        std::cin >> val;
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(32767, '\n');
            std::cerr << "Input Error! Try again: ";
            continue;
        }
        std::cin.ignore(32767, '\n');
        return val;
    }
}


int selectOperation(int& auth)
{
	cout<<"Select operaton: "<<endl;
	int operation_id=0; //перечисление сюда
	while (true)
    {
		if(auth==1)
        {
            cout <<"[1] Sell goods"<<endl;
            cout <<"[2] Show balance"<<endl;
            cout <<"[3] Show item detailed info"<<endl;
        }
        else
        if(auth==2)
        {
            cout <<"[1] Make new order"<<endl;
            cout <<"[2] Check order status/details"<<endl;
        }

		operation_id=validationInput();

		switch(operation_id)
		{
		    case 1: return 1; break;
            case 2: return 2; break;
            case 3: return 3; break;
            default: return 0; break;
		}

	}
}

int main(int argc, char* argv[])//что-то придумать с флагами?
{

    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        cout << "Error" << endl;
        exit(1);
    }

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0)
    {
        cout << "Failed connect to server" << endl;
        return 1;
    }
    else
    {
        cout << "Connected to server!" << endl;
    }

    char choice;
    string user_name_from_db;
    int user_role_from_db;
    //authorization(user_name_from_db, user_role_from_db);

    cout<<"Enter 1 to authorization menu or"<<endl;
    cout<<"      2 to registration menu: ";

    switch(validationInput())
    {
        case 1: authorization(user_name_from_db, user_role_from_db); break;
        case 2: registration(); break;

        default: cout<<"No such operation"<<endl; return 0;
    }



    if(user_role_from_db==1)
    {
        cout<<"Manager "<<user_name_from_db<<" console menu"<<endl;
    do
    {
        switch(selectOperation(user_role_from_db))
        {
            case 1: sell_goods(); break;
            case 2: show_inventory_balance(); break;
            case 3: show_item_detail_info(); break;
            //другой функцинал

            default: cout<<"No such operation"<<endl; break;
        }

    }while(makeChoice(choice));

    }
    else if(user_role_from_db==2)
    {
       cout<<"Customer "<<user_name_from_db<<" console menu"<<endl;

        do
        {
        switch(selectOperation(user_role_from_db))
        {
            case 1: make_order(); break;
            case 2: check_order_status(); break;
            //другой функцинал

            default: cout<<"No such operation"<<endl; break;
        }

        }while(makeChoice(choice));
    }
    else
        cout<<"Wrong user name or password"<<endl;

    system("pause");
    return 0;
}
