#include <gtk/gtk.h>
#include <sqlite3.h>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <regex>
#include "encrypt.hpp"
#include "utilities.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::regex;
using std::string;
using std::to_string;

Encryptor encryptor;

static void on_activate(GtkApplication *app, gpointer user_data);
static void on_logout_button_clicked(GtkWidget *widget, gpointer data);
void show_main_menu(GtkWindow *parent_window, string fname, double balance);
void LogIn(sqlite3 *db, const string &phoneNum, const string &password, GtkWindow *parent_window);
void SignUp(sqlite3 *db, const string &fname, const string &lname, const string &phoneNum, const string &password, GtkWindow *parent_window);

void show_main_menu
(GtkWindow *parent_window, std::string fname, double balance)
{
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *balance_label;
    GtkWidget *logout_button;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *spacer;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), "Main Menu");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), spacer, TRUE, TRUE, 0);

    logout_button = gtk_button_new_with_label("Logout");
    gtk_box_pack_end(GTK_BOX(hbox), logout_button, FALSE, FALSE, 0);

    g_signal_connect(logout_button, "clicked", G_CALLBACK(on_logout_button_clicked), window);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    std::string welcomeMSG = "Welcome " + fname + "!";
    label = gtk_label_new(welcomeMSG.c_str());

    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 10);

    std::stringstream balance_ss;
    balance_ss << std::fixed << std::setprecision(2) << balance;
    std::string balanceFormatted = balance_ss.str();
    std::string balanceMSG = "Your Balance: $" + balanceFormatted;
    balance_label = gtk_label_new(balanceMSG.c_str());
    gtk_box_pack_start(GTK_BOX(vbox), balance_label, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget *, GdkEvent *, gpointer) -> gboolean {
                         exit(0);
                         return FALSE;
                     }),
                     NULL);

    gtk_widget_hide(GTK_WIDGET(parent_window));
}

static void on_logout_button_clicked
(GtkWidget *widget, gpointer data)
{
    GtkWindow *main_menu_window = GTK_WINDOW(data);

    gtk_widget_hide(GTK_WIDGET(main_menu_window));

    GApplication *app = g_application_get_default();
    if (app) {
        on_activate(GTK_APPLICATION(app), NULL);
    }

    show_message_dialog(main_menu_window, "Logout Successful!");
}

void LogIn
(sqlite3 *db, const string &phoneNum, const string &password, GtkWindow *parent_window)
{
    string cleanedPhoneNum = cleanNumber(phoneNum);
    std::string encryptedPhoneNum = encryptor.encrypt(cleanedPhoneNum);
    std::string encryptedPassword = encryptor.encrypt(password);

    string checkUser = "SELECT COUNT(*) FROM LogIn WHERE PhoneNum = ? AND Password = ?;";
    string getFname = "SELECT FirstName FROM LogIn WHERE PhoneNum = ? AND Password = ?;";
    string getBalance = "SELECT balAMT FROM Balance WHERE AccNum = (SELECT AccNum FROM LogIn WHERE PhoneNum = ? AND Password = ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, checkUser.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cerr << "[SQL ERROR]: Check LogIn Info << " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Check LogIn Info.");
        return;
    }

    sqlite3_bind_text(stmt, 1, encryptedPhoneNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encryptedPassword.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        if (count > 0) {
            rc = sqlite3_prepare_v2(db, getFname.c_str(), -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                cerr << "[SQL ERROR]: Get fName << " << sqlite3_errmsg(db) << endl;
                show_message_dialog(parent_window, "SQL error: Get fName.");
                return;
            }

            sqlite3_bind_text(stmt, 1, encryptedPhoneNum.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, encryptedPassword.c_str(), -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW) {
                const unsigned char *encryptedFname = sqlite3_column_text(stmt, 0);
                string fname = encryptor.decrypt(reinterpret_cast<const char *>(encryptedFname));
                sqlite3_finalize(stmt);

                double balance = 0.0;
                rc = sqlite3_prepare_v2(db, getBalance.c_str(), -1, &stmt, NULL);
                if (rc != SQLITE_OK) {
                    cerr << "SQL error (prepare get balance): " << sqlite3_errmsg(db) << endl;
                    show_message_dialog(parent_window, "SQL error: Unable to prepare balance retrieval.");
                    return;
                }

                sqlite3_bind_text(stmt, 1, encryptedPhoneNum.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, encryptedPassword.c_str(), -1, SQLITE_STATIC);

                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    const unsigned char *encryptedBalance = sqlite3_column_text(stmt, 0);
                    string decryptedBalStr = encryptor.decrypt(reinterpret_cast<const char *>(encryptedBalance));
                    balance = std::stod(decryptedBalStr);
                } else {
                    cerr << "SQL error (get balance): " << sqlite3_errmsg(db) << endl;
                    show_message_dialog(parent_window, "SQL error: Unable to get balance.");
                }

                sqlite3_finalize(stmt);

                show_message_dialog(parent_window, "LogIn Successful!");
                show_main_menu(parent_window, fname, balance);
            } else {
                cerr << "SQL error (get fName): " << sqlite3_errmsg(db) << endl;
                show_message_dialog(parent_window, "SQL error: Unable to get fName.");
            }
        } else {
            show_message_dialog(parent_window, "Invalid phone number or password.");
        }
    } else {
        cerr << "SQL error (check user): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to check user.");
    }
}

void SignUp
(sqlite3 *db, const string &fname, const string &lname, const string &phoneNum, const string &password, GtkWindow *parent_window)
{
    std::string cleanedPhoneNum = cleanNumber(phoneNum);
    std::string encryptedPhoneNum = encryptor.encrypt(cleanedPhoneNum);

    const char *checkPhoneNum = "SELECT COUNT(*) FROM LogIn WHERE PhoneNum = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, checkPhoneNum, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cerr << "SQL error (prepare check phone number): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to check phone number.");
        return;
    }

    sqlite3_bind_text(stmt, 1, encryptedPhoneNum.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        if (count > 0) {
            show_message_dialog(parent_window, "Phone number already has account. Please use a different phone number or login.");
            sqlite3_finalize(stmt);
            return;
        }
    } else {
        cerr << "SQL error (check phone number): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to check phone number.");
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    std::string capitalizedFname = capitalizeFirstLetter(fname);
    std::string capitalizedLname = capitalizeFirstLetter(lname);

    std::string encryptedLname = encryptor.encrypt(capitalizedLname);
    std::string encryptedPassword = encryptor.encrypt(password);
    std::string encryptedFname = encryptor.encrypt(capitalizedFname);

    srand(time(0));

    int accNum = rand() % 900000 + 100000;
    std::string encryptedAccNum = encryptor.encrypt(to_string(accNum));

    int routNum = rand() % 900000 + 100000;
    std::string encryptedRoutNum = encryptor.encrypt(to_string(routNum));

    const char *addNewUser = "INSERT INTO LogIn (AccNum, RoutNum, LastName, FirstName, PhoneNum, Password) VALUES (?, ?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, addNewUser, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cerr << "SQL error (prepare insert): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to create account.");
        return;
    }

    sqlite3_bind_text(stmt, 1, encryptedAccNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encryptedRoutNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, encryptedLname.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, encryptedFname.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, encryptedPhoneNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, encryptedPassword.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "SQL error (insert user): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to create account.");
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);

    const char *addBalance = "INSERT INTO Balance (AccNum, balAMT) VALUES (?, ?);";
    rc = sqlite3_prepare_v2(db, addBalance, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cerr << "SQL error (prepare insert balance): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to set initial balance.");
        return;
    }

    double initialBalance = 1000.00;
    std::string encryptedBal = encryptor.encrypt(to_string(initialBalance));
    sqlite3_bind_text(stmt, 1, encryptedAccNum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, encryptedBal.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "SQL error (insert balance): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to set initial balance.");
    } else {
        show_message_dialog(parent_window, "Sign Up Successful!");
    }

    sqlite3_finalize(stmt);
}

static void on_login_button_clicked
(GtkWidget *widget, gpointer data)
{
    GtkWindow *parent_window = GTK_WINDOW(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Login", parent_window, GTK_DIALOG_MODAL, ("_OK"), GTK_RESPONSE_OK, ("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *phone_label = gtk_label_new("Phone Number:");
    GtkWidget *phone_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), phone_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), phone_entry, FALSE, FALSE, 5);

    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(content_area), password_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), password_entry, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *phone_num = gtk_entry_get_text(GTK_ENTRY(phone_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

        if (isValidPhoneNumber(phone_num)) {
            sqlite3 *db;
            int rc = sqlite3_open("login.db", &db);
            if (rc) {
                cerr << "[ERROR] Unable To Open Database: " << sqlite3_errmsg(db) << endl;
                show_message_dialog(parent_window, "Unable to open database.");
            } else {
                LogIn(db, phone_num, password, parent_window);
                sqlite3_close(db);
            }
        } else {
            show_message_dialog(parent_window, "Invalid phone number format.");
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_signup_button_clicked
(GtkWidget *widget, gpointer data)
{
    GtkWindow *parent_window = GTK_WINDOW(data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Sign Up", parent_window, GTK_DIALOG_MODAL, ("_OK"), GTK_RESPONSE_OK, ("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *fname_label = gtk_label_new("First Name:");
    GtkWidget *fname_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), fname_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), fname_entry, FALSE, FALSE, 5);

    GtkWidget *lname_label = gtk_label_new("Last Name:");
    GtkWidget *lname_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), lname_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), lname_entry, FALSE, FALSE, 5);

    GtkWidget *phone_label = gtk_label_new("Phone Number:");
    GtkWidget *phone_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), phone_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), phone_entry, FALSE, FALSE, 5);

    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(content_area), password_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content_area), password_entry, FALSE, FALSE, 5);

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *fname = gtk_entry_get_text(GTK_ENTRY(fname_entry));
        const char *lname = gtk_entry_get_text(GTK_ENTRY(lname_entry));
        const char *phone_num = gtk_entry_get_text(GTK_ENTRY(phone_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

        if (strlen(fname) == 0 || strlen(lname) == 0 || strlen(phone_num) == 0 || strlen(password) == 0) {
            show_message_dialog(parent_window, "All fields are required.");
        } else if (isValidPhoneNumber(phone_num)) {
            sqlite3 *db;
            int rc = sqlite3_open("login.db", &db);
            if (rc) {
                cerr << "[ERROR] Unable To Open Database: " << sqlite3_errmsg(db) << endl;
                show_message_dialog(parent_window, "Unable to open database.");
            } else {
                SignUp(db, fname, lname, phone_num, password, parent_window);
                sqlite3_close(db);
            }
        } else {
            show_message_dialog(parent_window, "Invalid phone number format.");
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc = sqlite3_open("login.db", &db);
    if (rc) {
        cerr << "[ERROR] Unable To Open Database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return;
    }

    const char *sql_create_table = "CREATE TABLE IF NOT EXISTS LogIn ("
                                   "AccNum INTEGER NOT NULL,"
                                   "RoutNum INTEGER NOT NULL,"
                                   "LastName TEXT NOT NULL,"
                                   "FirstName TEXT NOT NULL,"
                                   "PhoneNum TEXT NOT NULL,"
                                   "Password TEXT NOT NULL,"
                                   "PRIMARY KEY (AccNum, RoutNum, PhoneNum)"
                                   ");";

    rc = sqlite3_exec(db, sql_create_table, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error (create login table): " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return;
    }

const char *sql_create_bal_table = "CREATE TABLE IF NOT EXISTS Balance ("
                                   "AccNum INTEGER NOT NULL,"
                                   "balAMT DECIMAL NOT NULL,"
                                   "FOREIGN KEY (AccNum) REFERENCES LogIn(AccNum)"
                                   ");";

     rc = sqlite3_exec(db, sql_create_bal_table, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error (create bal table): " << zErrMsg << endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return;
    }

    GtkWidget *window;
    GtkWidget *outer_box;
    GtkWidget *button_box;
    GtkWidget *label_box;
    GtkWidget *login_button;
    GtkWidget *signup_button;
    GtkWidget *label;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "FinanceFirst");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    outer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), outer_box);

    label_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(outer_box), label_box, FALSE, FALSE, 0);

    label = gtk_label_new("<span font='25'><span foreground='#FFA500'>Finance</span><span foreground='#0000FF'>First</span></span>");
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);
    gtk_box_pack_start(GTK_BOX(label_box), label, FALSE, FALSE, 50);

    button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(button_box), FALSE);

    gtk_widget_set_valign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    login_button = gtk_button_new_with_label("Login");
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), window);
    gtk_box_pack_start(GTK_BOX(button_box), login_button, TRUE, TRUE, 0);

    signup_button = gtk_button_new_with_label("Sign Up");
    g_signal_connect(signup_button, "clicked", G_CALLBACK(on_signup_button_clicked), window);
    gtk_box_pack_start(GTK_BOX(button_box), signup_button, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(outer_box), button_box, TRUE, TRUE, 0);

    gtk_widget_show_all(window);

    sqlite3_close(db);
}

int main
(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.FinanceFirst", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}