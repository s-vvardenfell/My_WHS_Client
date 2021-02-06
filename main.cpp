#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <fstream>
using namespace std;

#define PORT 1111
#define SERVERADDR "127.0.0.1"
#define NO_ITEM "000"

SOCKET Connection;

int validationInput();
float validationInput(float);
bool makeChoice(char& choice, int version, string &msg);

enum Make_Choice_Versions
{
    LOCAL=1,
    SERVER,
};

enum Request_Codes
{
    AUTHORIZATION = 11111,
    CHECK_BALANCE = 22222,
    ITEM_DETAILED_INFO = 33333,
    SELL_MENU = 44444,
    REGISTRATION = 55555,
    CHECK_ORDER_STATUS = 66666,
    USER_EXIT = 88888,
    ADD_ITEMS_TO_DB = 99999

};

enum User_Roles
{
    MANAGER = 1,
    CUSTOMER = 2,
    ADMIN = 3,
    NO_USER = 0
};

void show_item_detail_info()
{
    cout<<"===showing item detailed info==="<<endl;

    string request_code=to_string(ITEM_DETAILED_INFO);
    int msg_size=request_code.size();
//    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    cout<<"Enter item code: "<<endl;
    int i_c = validationInput();
    string str_item_code = to_string(i_c);
    msg_size=str_item_code.size();
//    send(Connection, (char*)&msg_size, sizeof(int), NULL);
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, str_item_code.c_str(), msg_size, NULL);

//    recv(Connection, (char*)&msg_size, sizeof(int), NULL);
    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
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
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
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
    stringstream ss_order;
    char choice;
    string temp_str;

    string request_code=to_string(SELL_MENU);
    int msg_size=request_code.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    string message = "add another item?";
    //цикл набора заказа
    do
    {
    cout<<"Enter item code: ";

    int it_code = validationInput();
    string str_item_code = to_string(it_code);
    msg_size=str_item_code.size();

    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, str_item_code.c_str(), msg_size, NULL);

    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* records_from_table = new char[msg_size + 1];
    records_from_table[msg_size] = '\0';
    recv(Connection, records_from_table, msg_size, NULL);

    cout<<"records_from_table: "<<records_from_table<<endl;

    temp_str = records_from_table;

    delete[] records_from_table;

    if(temp_str==NO_ITEM)
    {
        cout<<"no such item code"<<endl;
        continue;
    }

    string stock_goods = temp_str.substr(0, temp_str.find('*'));
    temp_str.erase(0, temp_str.find('*')+1);

    string stock_amount = temp_str.substr(0, temp_str.find('*'));
    temp_str.erase(0, temp_str.find('*')+1);

    string stock_price = temp_str.erase(temp_str.size()-1);//deleting '\n'


    cout<<"There is "<<stock_amount<<" \""<<stock_goods<<"\" worth "<<stock_price<<" rubles each."<<endl;
    cout<<"Enter quantity for the order: ";

    bool notEnough;
    float amnt;

        do
        {
            amnt = validationInput(1.1);

            if(amnt>atof(stock_amount.c_str()))
            {
                cout<<"There is not enough product in stock for your order. Add a smaller amount."<<endl;
                notEnough=true;
            }
            else
            {
                notEnough=false;
            }

        }while(notEnough);

        ss_order<<str_item_code<<"*"<<to_string(amnt)<<"_";
        cout<<endl;
        cout<<"Product was added to your order"<<endl;

        temp_str=ss_order.str();
        cout<<"In your order:"<<endl;
        cout<<temp_str<<endl;

    }while(temp_str=="000" ? makeChoice(choice, LOCAL, message) : makeChoice(choice, SERVER, message));// отправляет на сервер сообщение о продолжении ввода

    cout<<"Your final order: "<<endl;
    cout<<temp_str<<endl;

    //отправляем на сервер строку, содержащую заказ
    msg_size=temp_str.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, temp_str.c_str(), msg_size, NULL);

    //получаем общую стоимость всего заказа
    cout<<"Total cost of the order: ";
    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* order_total_cost = new char[msg_size + 1];
    order_total_cost[msg_size] = '\0';
    recv(Connection, order_total_cost, msg_size, NULL);
    cout<<order_total_cost<<endl;

    delete[] order_total_cost;
    cout<<"Confirm order?"<<endl;
    //получаем имя и адрес клиента
    string str, customer_name, customer_address;

    bool gotName=false,
         gotAddress=false;

    message = "";
    if(makeChoice(choice, LOCAL, message))
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
        send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        send(Connection, temp_str.c_str(), msg_size, NULL);
    }
    else
    {
        cout<<"deleting temporary data"<<endl;//отправляем запрос на удаление временного заказа
        temp_str="@denied@";
        msg_size=temp_str.size();
        send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
        send(Connection, temp_str.c_str(), msg_size, NULL);
        return;
    }

        //принимаю данные всего заказа для формирования чека
        recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
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
            if (str[nIndex] == '_')
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
    int msg_size=request_code.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    //отправляю логин*пароль
    string login_and_password = str_login+"*"+str_pasw;
    msg_size=login_and_password.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, login_and_password.c_str(), msg_size, NULL);

    //получаю роль пользователя; если 0, то меню в main не активируется
    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* user_role_from_bd = new char[msg_size + 1];
    user_role_from_bd[msg_size] = '\0';
    recv(Connection, user_role_from_bd, msg_size, NULL);
    user_role=atoi(user_role_from_bd);

    cout<<"user_role_from_bd: "<<user_role_from_bd<<endl;
    delete[] user_role_from_bd;


}

void registration(string& user_name, int& user_role)
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
            if (str[nIndex] == '_')
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

        if (!bRejected&&!gotLogin)
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

    user_name=str_login;

    string request_code=to_string(REGISTRATION);
    int msg_size=request_code.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    string name_and_passw = str_login+"*"+str_pasw;
    cout<<"name_and_passw: "<<name_and_passw<<endl;
    msg_size=name_and_passw.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, name_and_passw.c_str(), msg_size, NULL);

    //getting user_role from serer: 0 if not successfully, 1,3 for manager, administrator, 2 for customer
    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* registration_confirm_by_user_role = new char[msg_size + 1];
    registration_confirm_by_user_role[msg_size] = '\0';
    recv(Connection, registration_confirm_by_user_role, msg_size, NULL);

    user_role = atoi(registration_confirm_by_user_role);

    if(user_role)
    {
        cout<<"User "<<user_name<<" had registered successfully"<<endl;
    }
    else
    {
        cout<<"Registration error"<<endl;
    }


}

void check_order_status()//принимает инт номер заказа возвр 0 если нет заказа; иначе заказ в подробностях
{
    cout<<"===checking order status==="<<endl;

    //sending request code
    string request_code=to_string(CHECK_ORDER_STATUS);
    int msg_size=request_code.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    //getting from client and sending order number to server
    cout<<"Enter order number: ";
    int order_number = validationInput();
    string str_order_number = to_string(order_number);///мб можно без создания временной string
    msg_size = str_order_number.size();

    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, str_order_number.c_str(), msg_size, NULL);
    cout<<endl;

    recv(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    char* order_details = new char[msg_size + 1];
    order_details[msg_size] = '\0';
    recv(Connection, order_details, msg_size, NULL);

    cout<<order_details<<endl;

    delete[] order_details;
}

//добавить enum на версии: in_order, simple, in_server
bool makeChoice(char& choice, int version, string &msg) // предлагаем повторить
{
	int msg_size;
	string continue_order;
	cout << "Do you want ";

	if(version==LOCAL)
    {
        while (true)
        {
            cout << "to continue? yes/no - y/n : ";
            cin >> choice;
            cin.ignore(SHRT_MAX, '\n');
            if (choice == 'n' || choice == 'y') return (choice == 'y') ? true : false;
            else cout << "Error, do you want ";
        }
    }
	else if(version==SERVER)
    {
        while (true)
        {
            cout<<msg<<endl;
            //cout << "add another item? yes/no - y/n : ";
            cin >> choice;
            cin.ignore(SHRT_MAX, '\n');

            if (choice == 'y')
            {
                continue_order = "1";
                msg_size=continue_order.size();
                send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connection, continue_order.c_str(), msg_size, NULL);

                return true;
            }
            else if(choice == 'n')
            {
                continue_order = "0";
                msg_size=continue_order.size();
                send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
                send(Connection, continue_order.c_str(), msg_size, NULL);

                return false;
            }
            else cout << "Input error, do you want ";

        }
    }


}

void add_items_to_db_from_file()
{
    cout<<"=======adding items to db from file (txt/xml/csv)=========="<<endl;
    //sending request code
//    string request_code=to_string(ADD_ITEMS_TO_DB);
//    int msg_size=request_code.size();
//
//    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
//    send(Connection, request_code.c_str(), msg_size, NULL);

    //suggesting enter the file name and selecting the separator

    cout<<"Enter file name (with .csv/.txt/.xml): ";

    string file_name, temp_str;

    while(true)
    {

        cout<<"Enter file name: "<<endl;
        getline(cin, temp_str);

        bool bRejected = false;

        for (unsigned int nIndex = 0; nIndex < temp_str.length() && !bRejected; ++nIndex)
        {
            if (isalpha(temp_str[nIndex]))
                continue;
            if (isdigit(temp_str[nIndex]))
                continue;
            if (isspace(temp_str[nIndex]))
                continue;
            if (temp_str[nIndex] == '_')
                continue;
            if (temp_str[nIndex] == '.')
                continue;
            else
            {
                cout<<"Invalid input"<<endl;
                bRejected = true;
            }
        }

        if (!bRejected)
        {
            file_name=temp_str;
            bRejected=false;
            break;
        }
    }

    cout<<"file_name: "<<file_name<<endl;

    if(1/*если имя файла содержит .txt/.csv */)
    {
        //reading the file
        ifstream file(file_name);

        if(!file)
        {
        cerr<<"File couldn't be opened"<<endl;
        cerr<<"Error: "<<strerror(errno)<<endl;
        }

        string data_from_file;

        while(getline(file,temp_str))
        {
            data_from_file.append(temp_str).append("\n");
        }

        file.close();

        cout<<data_from_file<<endl;

        //отправляем данные
    }
    else if(0 /*если имя файла содержит .xml*/)
    {
        //отправляем данные
    }

    //получаем ответ что всё ок и сколько загружено






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

float validationInput(float)
{
    float val;
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
		if(auth==MANAGER)
        {
            cout <<"[1] Sell goods"<<endl;
            cout <<"[2] Show balance"<<endl;
            cout <<"[3] Show item detailed info"<<endl;
            cout <<"[4] Add items to db from file (txt/xml)"<<endl;
            cout <<"[5] Exit"<<endl;
        }
        else
        if(auth==CUSTOMER)
        {

            cout <<"[1] Check order status/details"<<endl;
            cout <<"[2] Exit"<<endl;
            //cout <<"[3] Make new order"<<endl; //deleted
        }

		operation_id=validationInput();

		switch(operation_id)
		{
		    case 1: return 1; break;
            case 2: return 2; break;
            case 3: return 3; break;
            case 4: return 4; break;
            case 5: return 5; break;
            default: return 0; break;
		}

	}
}

int exit_from_client()
{
    cout<<"=======Exiting from client==========="<<endl;

    string request_code=to_string(USER_EXIT);
    int msg_size=request_code.size();
    send(Connection, reinterpret_cast<char*>(&msg_size), sizeof(int), NULL);
    send(Connection, request_code.c_str(), msg_size, NULL);

    shutdown(Connection, SD_BOTH);

    closesocket(Connection);
    WSACleanup();


    exit(0);

}

int main(int argc, char* argv[])
{

    WSADATA wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);

    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        cout<<"WSAStartup error, "<<WSAGetLastError()<<endl;
        return -1;
    }

    Connection = socket(AF_INET, SOCK_STREAM, NULL);

    if(Connection<0)
    {
        cout<<"Function socket() error, "<<WSAGetLastError()<<endl;
        return -1;
    }


    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    HOSTENT *hst;
    int addr_size = sizeof(addr);

    if(inet_addr(SERVERADDR)!= INADDR_NONE)
    {
        addr.sin_addr.s_addr = inet_addr(SERVERADDR);
    }
    else
    {
        if(hst = gethostbyname(SERVERADDR))
            ((unsigned long*)&addr.sin_addr)[0] = ((unsigned long**)hst->h_addr_list)[0][0];
        else
        {
            cout<<"Invalid address: "<<SERVERADDR<<endl;
            closesocket(Connection);
            WSACleanup();
            return -1;
        }
    }

    if(connect(Connection,(sockaddr*)&addr,addr_size))
    {
        cout<<"Connection error: "<<WSAGetLastError()<<endl;
        return -1;
    }
    else
    {
        cout<<"Connected successfully with "<<SERVERADDR<<endl;
    }

// simply, but not so cool
//    SOCKADDR_IN addr;
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(1111);
//    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//
//    int sizeofaddr = sizeof(addr);
//
//    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0)
//    {
//        cout << "Failed connect to server" << endl;
//        return 1;
//    }
//    else
//    {
//        cout << "Connected to server!" << endl;
//    }

    char choice;
    string user_name_from_db;
    int user_role_from_db=NO_USER;

    string message = "";

    cout<<"Enter 1 to authorization menu or"<<endl;
    cout<<"      2 to registration menu: ";

    switch(validationInput())
    {
        case 1: authorization(user_name_from_db, user_role_from_db); break;
        case 2: registration(user_name_from_db, user_role_from_db); break;

        default: cout<<"No such operation"<<endl; /*exit_from_client();*/ break;
    }

    if(user_role_from_db==MANAGER)
    {
        cout<<"Manager "<<user_name_from_db<<" console menu"<<endl;
        do
        {
            switch(selectOperation(user_role_from_db))
            {
                case 1: sell_goods(); break;
                case 2: show_inventory_balance(); break;
                case 3: show_item_detail_info(); break;
                case 4: add_items_to_db_from_file(); break;
                case 5: exit_from_client(); break;
                //any other functions

                default: cout<<"No such operation"<<endl; break;
            }

        }while(makeChoice(choice, LOCAL, message));

    }
    else if(user_role_from_db==CUSTOMER)
    {
       cout<<"Customer "<<user_name_from_db<<" console menu"<<endl;

        do
        {
            switch(selectOperation(user_role_from_db))
            {

                case 1: check_order_status(); break;
                case 2: exit_from_client(); break;
                //case 4: make_order(); break; //for instance; any other functions

                default: cout<<"No such operation"<<endl; break;
            }

        }while(makeChoice(choice, LOCAL, message));
    }
    else
    {
        cout<<"Wrong user name or password"<<endl;
    }

    system("pause");

    return 0;
}
