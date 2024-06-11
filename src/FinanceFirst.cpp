#include <gtk/gtk.h>
#include <iostream>
#include <sqlite3.h>
#include <ctime>
#include <regex>
#include <limits>
#include <algorithm>
#include "encrypt.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::regex;
using std::string;

Encryptor encryptor;

static void show_message_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

bool isValidPhoneNumber(const string phoneNum) {
    regex pattern(R"(^(\d{3}[-. ]?)?\d{3}[-. ]?\d{4}$)");
    return regex_match(phoneNum, pattern);
}

string cleanNumber(const string& phoneNum) {
    string phone;
    std::copy_if(phoneNum.begin(), phoneNum.end(), std::back_inserter(phone), ::isdigit);
    return phone;
}

void show_main_menu(GtkWindow *parent_window) {
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *vbox;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), "Main Menu");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    label = gtk_label_new("Welcome to the main menu!");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer) -> gboolean {
        exit(0);
        return FALSE;
    }), NULL);

    gtk_widget_hide(GTK_WIDGET(parent_window));
}

void LogIn(sqlite3* db, const string& phoneNum, const string& password, GtkWindow *parent_window) {
    string cleanedPhoneNum = cleanNumber(phoneNum);
    std::string encryptedPhoneNum = encryptor.encrypt(cleanedPhoneNum);
    std::string encryptedPassword = encryptor.encrypt(password);

    string checkUser = "SELECT COUNT(*) FROM LogIn WHERE PhoneNum = ? AND Password = ?;";

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
        if (count > 0) {
            show_message_dialog(parent_window, "LogIn Successful!");
            show_main_menu(parent_window);
        } else {
            show_message_dialog(parent_window, "Invalid phone number or password.");
        }
    } else {
        cerr << "SQL error (check user): " << sqlite3_errmsg(db) << endl;
        show_message_dialog(parent_window, "SQL error: Unable to check user.");
    }

    sqlite3_finalize(stmt);
}

void SignUp(sqlite3* db, const string& fname, const string& lname, const string& phoneNum, const string& password, GtkWindow *parent_window) {
    std::string cleanedPhoneNum = cleanNumber(phoneNum);
    std::string encryptedPhoneNum = encryptor.encrypt(cleanedPhoneNum);

    // Check if the phone number already exists in the database
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

    std::string encryptedLname = encryptor.encrypt(lname);
    std::string encryptedPassword = encryptor.encrypt(password);
    std::string encryptedFname = encryptor.encrypt(fname);

    srand(time(0));

    int accNum = rand() % 900000 + 100000;
    std::string encryptedAccNum = encryptor.encrypt(std::to_string(accNum));

    int routNum = rand() % 900000 + 100000;
    std::string encryptedRoutNum = encryptor.encrypt(std::to_string(routNum));

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
    } else {
        show_message_dialog(parent_window, "Sign Up Successful!");
    }

    sqlite3_finalize(stmt);
}

static void on_login_button_clicked(GtkWidget *widget, gpointer data) {
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

static void on_signup_button_clicked(GtkWidget *widget, gpointer data) {
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

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *fname = gtk_entry_get_text(GTK_ENTRY(fname_entry));
        const char *lname = gtk_entry_get_text(GTK_ENTRY(lname_entry));
        const char *phone_num = gtk_entry_get_text(GTK_ENTRY(phone_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

        if (isValidPhoneNumber(phone_num)) {
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
        cerr << "SQL error (create table): " << zErrMsg << endl;
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

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.MyGuiApp", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}