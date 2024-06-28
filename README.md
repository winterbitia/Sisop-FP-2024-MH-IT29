# Sisop-FP-2024-MH-IT29

## Anggota Kelompok:
- Dian Anggraeni Putri (5027231016)
- Amoes Noland (5027231028)
- Malvin Putra Rismahardian (5027231048)

# WORK IN PROGRESS !!!

# Pendahuluan
Dalam final project praktikum DiscorIT, kami diminta untuk menyelesaikan implementasi sebuah sistem chat berbasis socket yang terdiri dari tiga file utama yaitu discorit.c (client untuk mengirim request), server.c (server yang menerima dan merespon request), dan monitor.c (client untuk menampilkan chat secara real-time).

Program ini memungkinkan user untuk berkomunikasi secara real-time melalui channel dan room yang dapat dikelola oleh user dengan peran tertentu. User harus melakukan autentikasi sebelum dapat mengakses fitur-fitur yang ada. Keamanan juga dijamin dengan menggunakan bcrypt untuk enkripsi password dan key channel.

### Tree
![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256172451142963303/Screenshot_1970.png?ex=667fcd42&is=667e7bc2&hm=d495f45000b7043dbf785212aaff8bc655e57391623ec388db5340a0f382f399&=&format=webp&quality=lossless&width=718&height=656)

### Keterangan Setiap File
![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256172451428171947/Screenshot_1971.png?ex=667fcd42&is=667e7bc2&hm=a6bea88f079dc47b94bb9ee4fe0ad4180a1f7d29cf8c5219be8fb77a16eef59e&=&format=webp&quality=lossless&width=584&height=656)

# Authentikasi

## Login/Register (Client)
Langkah pertama dalam proses autentikasi adalah menghubungkan client ke server. Ini dilakukan dengan menggunakan fungsi connect_server yang membuat socket dan menghubungkannya ke alamat IP dan port server.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
// Server connect function
void connect_server() {
    // Connect to server
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    // DEBUGGING
    printf("Connected to server\n");
}
```
</details>

### Pengiriman Register/Login ke Server
Setelah koneksi berhasil, client dapat mengirim command untuk register atau login. Command ini dikirim dalam bentuk string yang diformat sesuai kebutuhan server, kemudian dikirim melalui socket yang telah dibuat.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
// Main function
int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>\n(not enough arguments)");
        return 1;
    } 
    if (strcmp(argv[1], "REGISTER") != 0 && strcmp(argv[1], "LOGIN") != 0) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>\n(invalid command)");
        return 1;
    } 
    if (strcmp(argv[3], "-p") != 0) {
        printf("Usage: ./discorit REGISTER <username> -p <password>\n(missing -p flag)");
        return 1;
    }

    // Connect to server
    connect_server();

    // Prepare buffer
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    strcpy(username, argv[2]);
    strcpy(password, argv[4]);

    // Register user
    if (strcmp(argv[1], "REGISTER") == 0) {
        sprintf(buffer, "REGISTER %s %s", username, password);
        handle_account(buffer);
        close(server_fd);
        return 0;
    }

    // Login user
    if (strcmp(argv[1], "LOGIN") == 0) {
        sprintf(buffer, "LOGIN %s %s", username, password);
        if (handle_account(buffer) == 1)
        while(1){
            if (strlen(room) > 0) 
                printf("[%s/%s/%s] ", username, channel, room);
            else if (strlen(channel) > 0) 
                printf("[%s/%s] ", username, channel);
            else 
                printf("[%s] ", username);

            memset(buffer, 0, MAX_BUFFER);
            fgets(buffer, MAX_BUFFER, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            int res = handle_command(buffer);
            if (res == 2)
                return 0;
            if (res == 1)
                key_request(buffer);
        }
    }
}
```
</details>

### Pengelolaan Buffer dan Pengiriman Register/Login ke Server
Fungsi handle_account digunakan untuk mengirim command register atau login ke server, kemudian menerima dan menangani respon dari server.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
// Account handler function
int handle_account(const char *buffer) {
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    char response[MAX_BUFFER];
    memset(response, 0, MAX_BUFFER);
    if (recv(server_fd, response, MAX_BUFFER, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    if (strcmp(type, "MSG") == 0) {
        printf("%s\n", message);
        return 0;
    } else if (strcmp(type, "LOGIN") == 0) {
        printf("%s\n", message);
        return 1;
    }
}
```
</details>

## Login/Register (Server)
### Mendapatkan Folder Menggunakan getcwd
Awalnya server akan mendapat folder menggunakan fungsi getcwd. Ini digunakan untuk menyimpan dan mengakses file users.csv dan channels.csv.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
int main(int argc, char *argv[]){
    // Get current directory
    getcwd(cwd, sizeof(cwd));    
    sprintf(users_csv, "%s/users.csv", cwd);
    sprintf(channels_csv, "%s/channels.csv", cwd);

    // Start server
    start_server();

    // Foreground handling
    if (argc > 1 && strcmp(argv[1], "-f") == 0){
        printf("[SERVER] running in foreground!\n");
    } else daemonize();

    // Accept incoming connections
    while (1){
        if ((gclient_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Prepare client data
        client_data *client = (client_data *)malloc(sizeof(client_data));
        client->socket_fd = gclient_fd;
        memset(client->username, 0, MAX_BUFFER);
        memset(client->role, 0, 5);

        // Create thread for client
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)client);

        // Daemon sleep
        sleep(1);
    }
}
```
</details>

### Start Server
Fungsi `start_server` bertanggung jawab untuk menginisialisasi server dengan beberapa langkah penting. Pertama, fungsi ini membuat sebuah socket yang akan digunakan untuk komunikasi jaringan. Jika pembuatan socket gagal, program akan menampilkan pesan kesalahan dan keluar. Selanjutnya, socket tersebut diikat (bind) ke alamat dan port tertentu, memungkinkan server untuk menerima koneksi dari klien melalui port tersebut. Jika proses pengikatan gagal, program akan menampilkan pesan kesalahan dan keluar. Setelah socket berhasil diikat, server akan mendengarkan koneksi masuk dari klien, siap dalam menerima sejumlah koneksi yang telah ditentukan oleh `MAX_CLIENTS`. Jika proses ini gagal, program akan menampilkan pesan kesalahan dan keluar. Sebagai tambahan, server akan menampilkan pesan debug untuk menunjukkan bahwa server telah dimulai dan mendengarkan pada port yang ditentukan.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void start_server(){
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // DEBUGGING
    printf("[SERVER] Started on port %d\n", PORT);
}
```
</details>

### Daemon
Jika server dijalankan tanpa flag `-f`, maka server akan berjalan sebagai daemon. Fungsi `daemonize` bertugas untuk memisahkan proses dari terminal dan menjalankannya di latar belakang. Langkah pertama adalah melakukan fork untuk membuat child process, dan jika berhasil, parent process akan keluar sehingga hanya child process yang berjalan. Selanjutnya, umask diatur ke 0 untuk memastikan izin file yang benar, dan session ID baru dibuat dengan `setsid()` untuk memisahkan proses dari terminal pengendali. Folder diubah ke root (`/`) untuk menghindari penguncian folder tertentu. Setelah itu, file deskriptor standar (`stdin`, `stdout`, dan `stderr`) ditutup untuk memutus hubungan dengan terminal, dan kemudian diarahkan ke `/dev/null` untuk memastikan input/output daemon tidak mengganggu atau terganggu oleh terminal.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void daemonize(){
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);

    if ((chdir("/")) < 0) exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
}
```
</details>

### Menunggu Koneksi Client dan Membuat Thread
Untuk menunggu koneksi masuk dari klien dan membuat thread baru untuk menangani setiap koneksi, server menggunakan sebuah loop tak terbatas. Di setiap iterasi loop, server akan memanggil `accept()` untuk menerima koneksi dari klien. Jika koneksi diterima dengan sukses, server akan menyiapkan struktur data `client_data` untuk menyimpan informasi klien seperti file descriptor socket, username, dan peran (role). Selanjutnya, server membuat thread baru menggunakan `pthread_create()` yang akan menjalankan fungsi `handle_client` untuk menangani komunikasi dengan klien menggunakan data `client_data` yang telah disiapkan. Proses ini memungkinkan server untuk melayani beberapa klien secara bersamaan dengan menggunakan thread terpisah untuk setiap koneksi. Setelah membuat thread, server melakukan `sleep(1)` untuk memberi jeda sebelum kembali menunggu koneksi baru, menyesuaikan kecepatan proses dengan penggunaan sumber daya yang optimal.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
// Accept incoming connections
while (1){
    if ((gclient_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0){
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    // Prepare client data
    client_data *client = (client_data *)malloc(sizeof(client_data));
    client->socket_fd = gclient_fd;
    memset(client->username, 0, MAX_BUFFER);
    memset(client->role, 0, 5);

    // Create thread for client
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, (void *)client);

    // Daemon sleep
    sleep(1);
}
```
</details>

### Handling Buffer yang Dikirim Client untuk Register/Login
Untuk menangani buffer yang dikirim oleh klien untuk proses registrasi (REGISTER) atau masuk (LOGIN), fungsi handle_client berperan penting. Pertama, fungsi ini membaca data dari klien melalui socket menggunakan recv() dan menyimpannya dalam buffer buffer dengan ukuran maksimum MAX_BUFFER. Setelah menerima data, fungsi menggunakan strtok() untuk memisahkan perintah (command) dari buffer, yang kemudian dibandingkan dengan string "REGISTER" atau "LOGIN". Jika command sesuai dengan "REGISTER", fungsi handle_register dipanggil untuk memproses registrasi klien dengan data yang diterima. Sedangkan jika command adalah "LOGIN", fungsi handle_login dipanggil untuk memproses proses login klien. Setelah selesai memproses command, buffer direset menggunakan memset() untuk persiapan menerima data selanjutnya dari klien. Ketika klien menutup koneksi, handle_client menutup socket yang terkait, membebaskan memori yang dialokasikan untuk struktur data klien (client_data), dan mengembalikan nilai NULL karena tipe fungsi ini adalah void *
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void *handle_client(void *arg) {
    client_data *client = (client_data *)arg;
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    while (recv(client->socket_fd, buffer, MAX_BUFFER, 0) > 0) {
        char *command = strtok(buffer, " ");
        if (strcmp(command, "REGISTER") == 0) {
            handle_register(client, buffer);
        } else if (strcmp(command, "LOGIN") == 0) {
            handle_login(client, buffer);
        }
        memset(buffer, 0, MAX_BUFFER);
    }
    close(client->socket_fd);
    free(client);
    return NULL;
}
```
</details>

## Fungsi Register di Server
Fungsi register_user bertanggung jawab untuk menangani pendaftaran user baru. Fungsi ini memastikan bahwa username unik dan menyimpan data user yang baru terdaftar di users.csv. User pertama yang mendaftar akan diberi user ID 1 (root).
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void register_user(char *username, char *password, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER * 2];

    // Check if username has comma or is called USER
    if (strchr(username, ',') != NULL || strcmp(username, "USER") == 0) {
        // DEBUGGING
        printf("[REGISTER] Error: Username not allowed\n");

        // Send response to client
        sprintf(response, "MSG,Error: Username not allowed");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[REGISTER] Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and username
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("[REGISTER] id: %d, name: %s\n", id, namecheck);
        // sleep(1);

        // Fail if username already exists
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[REGISTER] Error: Username already exists\n");

            // Close file
            fclose(file);

            // Send response to client
            sprintf(response, "MSG,Error: Username %s already exists", username);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Hash password
    char hash[MAX_BUFFER];
    strcpy(hash, crypt(password, HASHCODE));

    // Set role to USER by default
    char role[8];
    if (id == 0) strcpy(role, "ROOT");
    else strcpy(role, "USER");

    // DEBUGGING
    printf("[REGISTER] id: %d, name: %s, pass: %s, role: %s\n", id+1, username, hash, role);
    
    // Write to file
    fprintf(file, "%d,%s,%s,%s\n", id+1, username, hash, role);
    fclose(file);

    // Send response to client
    sprintf(response, "MSG,Success: User %s registered", username);
    send(client_fd, response, strlen(response), 0);
}
```
</details>

User pertama yang mendaftar akan diberi user ID 1 dan dianggap sebagai root. Fungsi get_next_user_id akan mengembalikan user ID berikutnya berdasarkan ID tertinggi yang ada di users.csv.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
int get_next_user_id() {
    FILE *file = fopen(users_csv, "r");
    int max_id = 0;
    char line[MAX_BUFFER];
    while (fgets(line, MAX_BUFFER, file)) {
        int user_id = atoi(strtok(line, ","));
        if (user_id > max_id) {
            max_id = user_id;
        }
    }
    fclose(file);
    return max_id + 1;
}
```
</details>

Fungsi username_exists digunakan untuk memeriksa apakah username sudah ada di users.csv. Jika sudah ada, maka user tidak bisa mendaftar dengan username tersebut.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
bool username_exists(char *username) {
    FILE *file = fopen(users_csv, "r");
    char line[MAX_BUFFER];
    while (fgets(line, MAX_BUFFER, file)) {
        char *token = strtok(line, ",");
        strtok(NULL, ","); // Skip user ID
        char *existing_username = strtok(NULL, ",");
        if (strcmp(existing_username, username) == 0) {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}
```
</details>

Password yang diberikan oleh user akan dienkripsi sebelum disimpan di users.csv. Fungsi encrypt_password melakukan enkripsi sederhana dengan menggeser setiap karakter dalam password.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void encrypt_password(char *password, char *encrypted_password) {
    for (int i = 0; i < strlen(password); i++) {
        encrypted_password[i] = password[i] + 1;
    }
    encrypted_password[strlen(password)] = '\0';
}
```
</details>

## Fungsi Login di Server
Fungsi `login_user` pada server bertugas memverifikasi user yang mencoba login. Pertama, fungsi ini meng-hash password input menggunakan `crypt`. Kemudian, file `users.csv` yang berisi data user dibuka dalam mode read. Jika file tidak bisa dibuka, fungsi mengirim pesan error ke client. Selanjutnya, fungsi membaca data user dari file, mengecek apakah username yang diberikan ada di file. Jika ditemukan username yang cocok, fungsi memverifikasi apakah password yang di-hash cocok dengan hash yang disimpan. Jika cocok, data pengguna (ID, username, dan role) disimpan dalam struktur `client`, file ditutup, dan pesan sukses dikirim ke client. Jika password tidak cocok, atau username tidak ditemukan di file, fungsi mengirim pesan error yang sesuai ke client dan menutup file.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//============//
// LOGIN UsER //
//============//

int login_user(char *username, char *password, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Hash password from input
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));
    
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[LOGIN] Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    // Loop through id, username, and password
    int id = 0; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[LOGIN] id: %d, name: %s, pass: %s, role: %s\n", id, namecheck, passcheck, role);

        // Fail if username and password do not match
        if (strcmp(namecheck, username) == 0) {
            if (strcmp(passcheck, hash) == 0) {
                // DEBUGGING
                printf("[LOGIN] Success: Username and password match\n");

                // Finishing up
                client->id = id;
                strcpy(client->username, username);
                strcpy(client->role, role);
                fclose(file);

                // DEBUGGING
                printf("[LOGIN] client_id: %d, client_name: %s, client_role: %s\n", client->id, client->username, client->role);

                // Send response to client
                sprintf(response, "LOGIN,Welcome to DiscorIT!\n");
                send(client_fd, response, strlen(response), 0);
                return 1;
            }

            // DEBUGGING
            printf("[LOGIN] Error: Password does not match\n");

            // Fail if password does not match
            sprintf(response, "MSG,Error: Password does not match");
            send(client_fd, response, strlen(response), 0);
            fclose(file);
            return 0;
        }
    }

    // DEBUGGING
    printf("[LOGIN] Error: Username does not exist\n");

    // Send response to client
    sprintf(response, "MSG,Error: Username does not exist");
    send(client_fd, response, strlen(response), 0);
    fclose(file);
    return 0;
}
```
</details>

## Apa yang terjadi setelah Login

### Client

Setelah user berhasil login, fungsi `handle_account` akan mengirimkan buffer yang berisi informasi login ke server dan menerima respon. Respon dari server akan diparsing untuk memisahkan jenis pesan dan isi pesan. Jika respon adalah pesan biasa ("MSG"), pesan tersebut akan ditampilkan di layar. Jika respon adalah pesan login ("LOGIN"), pesan selamat datang akan ditampilkan. Setelah itu, program masuk ke dalam loop utama di `discorit`, dimana user dapat memasukkan perintah. Setiap perintah yang dimasukkan akan diproses oleh `handle_command`, yang mengirimkan perintah tersebut ke server dan menerima respon. Fungsi `handle_command` juga memproses jenis-jenis respon yang berbeda, seperti pesan biasa, perubahan nama channel atau room, keluar dari channel atau room, perubahan username, permintaan key, atau perintah keluar. Semua ini memungkinkan user untuk berinteraksi dengan server, berpindah channel atau room, mengubah username, dan mengirim serta menerima pesan dengan lancar.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//==================//
// COMMAND HANDLING //
//==================//

// Account handler function
int handle_account(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER];
    memset(response, 0, MAX_BUFFER);
    if (recv(server_fd, response, MAX_BUFFER, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Login status sending
    else if (strcmp(type, "LOGIN") == 0){
        printf("%s\n", message);
        return 1;
    }
}

// Command handler function
int handle_command(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER*8];
    memset(response, 0, MAX_BUFFER*8);
    if (recv(server_fd, response, MAX_BUFFER*8, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Channel/room variable sending
    else if (strcmp(type, "CHANNEL") == 0){
        // Parse channel name
        char *channel_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (channel_name == NULL) {
            perror("channel name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy channel name
        strcpy(channel, channel_name);
        printf("%s\n", message);
        return 0;
    } else if (strcmp(type, "ROOM") == 0){
        // Parse room name
        char *room_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (room_name == NULL) {
            perror("room name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy room name
        strcpy(room, room_name);
        printf("%s\n", message);
        return 0;
    }

    // Channel/room exiting
    else if (strcmp(type, "EXIT") == 0){
        // Parse exit type
        char *exit_type = strtok(NULL, ",");

        // Check if parsing is correct
        if (exit_type == NULL) {
            perror("exit type is empty");
            exit(EXIT_FAILURE);
        }

        // Check if exiting channel
        if (strcmp(exit_type, "CHANNEL") == 0) {
            memset(channel, 0, 100);
            // also reset room in case user is in a room
            memset(room, 0, 100);
            printf("%s\n", message);
            return 0;
        }

        // Check if exiting room
        else if (strcmp(exit_type, "ROOM") == 0) {
            memset(room, 0, 100);
            printf("%s\n", message);
            return 0;
        }
    }

    // Username change
    else if (strcmp(type, "USERNAME") == 0){
        // Parse new username
        char *new_username = strtok(NULL, ",");

        // Check if parsing is correct
        if (new_username == NULL) {
            perror("new username is empty");
            exit(EXIT_FAILURE);
        }

        // Copy new username
        strcpy(username, new_username);
        printf("%s\n", message);
        return 0;
    }

    // Key request
    else if (strcmp(type, "KEY") == 0){
        printf("%s\n", message);
        return 1;
    }

    // Quit message
    else if (strcmp(type, "QUIT") == 0){
        printf("%s\n", message);
        return 2;
    }
}
```
</details>

### Server
Fungsi `handle_client` bertanggung jawab untuk menangani koneksi dari setiap client yang terhubung ke server chat. Saat dipanggil, fungsi ini menerima socket client sebagai argumen. Pertama, socket client diambil dari argumen dan memori untuk argumen tersebut dibebaskan. Fungsi kemudian menambahkan socket client ke dalam daftar `clients` yang sedang aktif, menggunakan mutex untuk menjaga keamanan akses. Setelah itu, fungsi masuk ke dalam loop yang terus menerus menerima pesan dari client menggunakan `recv`. Setiap pesan yang diterima kemudian disebarkan ke semua client lain dengan fungsi `broadcast_message`. Jika koneksi dengan client terputus, fungsi ini menghapus client dari daftar `clients` dan menutup socket-nya sebelum berakhir.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//===============//
// HANDLE CLIENT //
//===============//

void *handle_client(void *arg){
    client_data *client = (client_data *)arg;
    int client_fd = client->socket_fd;

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // Receive data from client
    if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse data from client
    char *command = strtok(buffer, " ");
    char *username = strtok(NULL, " ");
    char *password = strtok(NULL, " ");

    // DEBUGGING
    printf("command: %s, username: %s, password: %s\n", command, username, password);

    // Register user
    if (strcmp(command, "REGISTER") == 0){
        // Call register user function
        register_user(username, password, client);

        // Close client connection
        close(client_fd);
        free(client);
        pthread_exit(NULL);
        return NULL;
    }

    // Login user
    else if (strcmp(command, "LOGIN") == 0){
        // Call login user function
        if (!login_user(username, password, client)){
            // Close client connection when login fails
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return NULL;
        }

        // DEBUGGING
        printf("[%s][LOGIN]\n", username);

        // Call handle input function
        handle_input(client);
    }
}
```
</details>

Setelah berhasil login, handle_input akan menunggu perintah dari client dengan terus mendengarkan data yang dikirimkan melalui socket. Perintah yang diterima akan diproses berdasarkan jenisnya, seperti "EXIT" untuk keluar, "SEE" untuk melihat informasi tertentu, "CREATE" untuk membuat channel atau room baru, dan lain-lain. Sebelum memproses perintah, fungsi ini akan memeriksa keberadaan dan status user untuk memastikan bahwa user tidak di-ban. Jika user di-ban, maka akan mengirimkan pesan error ke client dan menunggu perintah berikutnya. Setiap perintah yang valid akan memanggil fungsi terkait untuk menjalankan aksi yang diminta, seperti create_channel untuk perintah "CREATE CHANNEL" atau send_chat untuk perintah "CHAT". Ini memastikan bahwa setiap interaksi client dengan server ditangani dengan benar sesuai dengan perintah yang diberikan.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//==============//
// HANDLE INPUT //
//==============//

void handle_input(void *arg){
    client_data *client = (client_data *)arg;
    int client_fd = client->socket_fd;

    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER*2];

    while(1){
        // DEBUGGING
        printf("[%s] Waiting for command...\n", client->username);

        // Clear buffer
        memset(buffer, 0, MAX_BUFFER);

        // Receive data from client
        if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        // Check if user still exists
        if (!check_user(client)){
            // DEBUGGING
            printf("[%s] User does not exist\n", client->username);

            // Send response to client
            memset(response, 0, MAX_BUFFER * 2);
            sprintf(response, "QUIT,Error: User does not exist");
            send(client_fd, response, strlen(response), 0);

            // Close client connection
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return;
        }

        // Check if user is banned
        int ban = check_ban(client);
        if (ban == 1){
            // DEBUGGING
            printf("[%s] User is banned\n", client->username);

            // Update client channel and room
            memset(client->channel, 0, 100);
            memset(client->room, 0, 100);

            // Send response to client
            memset(response, 0, MAX_BUFFER * 2);
            sprintf(response, "EXIT,Error: User is banned,CHANNEL");
            send(client_fd, response, strlen(response), 0);

            // Accept next command
            continue;
        } else if (ban < 0){
            // DEBUGGING
            printf("[%s] Error: Unable to check ban\n", client->username);

            // Update client channel and room
            memset(client->channel, 0, 100);
            memset(client->room, 0, 100);

            // Send response to client
            memset(response, 0, MAX_BUFFER * 2);
            sprintf(response, "EXIT,Error: Unable to check ban,CHANNEL");
            send(client_fd, response, strlen(response), 0);

            // Accept next command
            continue;
        }

        // Prepare parse data from client
        char *command = strtok(buffer, " ");

        // Start command handling
        if (strcmp(command, "EXIT") == 0){
            // DEBUGGING
            printf("[%s][EXIT]\n", client->username);

            // Call exit user function
            exit_user(client);

 } else if (strcmp(command, "SEE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid see (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing see type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // See types
            if (strcmp(type, "USER") == 0){
                // Call print user function
                see_user(client);
            } else if (strcmp(type, "CHAT") == 0){
                // Call print chat function
                see_chat(client);
            } else {
                // DEBUGGING
                printf("[%s] Error: See type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: See type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "CREATE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid create (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing create type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Create types
            if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *key = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL || key == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing name/key)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing channel name/key)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                } else if (strcmp(flag, "-k") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid flag statement\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing -k flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Channel: %s, Key: %s\n", client->username, channel, key);

                // Call create channel function
                create_channel(channel, key, client);
            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *room = strtok(NULL, " ");

                // Check if command is valid
                if (room == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing room name)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing room name)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Room: %s\n", client->username, room);

                // Call create room function
                create_room(room, client);
            } else {
                // DEBUGGING
                printf("[%s] Error: Create type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Create type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "LIST") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid listing (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing list type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Listing types
            if (strcmp(type, "CHANNEL") == 0){
                // Call list channels function
                list_channel(client);
            } else if (strcmp(type, "ROOM") == 0){
                // Call list room function
                list_room(client);
            } else if (strcmp(type, "USER") == 0){
                // Call list user function
                list_user(client);
            } else {
                // DEBUGGING
                printf("[%s] Error: List type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: List type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "EDIT") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid edit (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing edit type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Edit types
            if (strcmp(type, "WHERE") == 0){
                char *target = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing target or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Target: %s, New: %s\n", client->username, flag, target, new);

                // Branch flag types
                if (strcmp(flag, "-u") == 0){
                    // Call edit username function
                    edit_username(target, new, client);
                } else if (strcmp(flag, "-p") == 0){
                    // Call edit password function
                    edit_password(target, new, client);
                } else {
                    // DEBUGGING
                    printf("[%s] Error: Flag type not found\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Flag type not found");
                    send(client_fd, response, strlen(response), 0);
                }
            } else if (strcmp(type, "CHAT") == 0) {
                // Parse data from client
                char *target = strtok(NULL, " ");
                char *message = strtok(NULL, "\n");

                // Check if command is valid
                if (target == NULL || message == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/message)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing target or message)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if message is correctly encased in quotes
                if (message[0] != '"' || message[strlen(message)-1] != '"'){
                    // DEBUGGING
                    printf("[%s] Error: Invalid message (missing quotes)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing quotes)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s, Message: %s\n", client->username, target, message);

                // call edit chat function
                edit_chat(atoi(target), message, client);
            } else if (strcmp(type, "PROFILE") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing target or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Target: %s, New: %s\n", client->username, flag, target, new);

                // Branch flag types
                if (strcmp(flag, "-u") == 0){
                    // Call edit username function
                    edit_username(client->username, new, client);
                } else if (strcmp(flag, "-p") == 0){
                    // Call edit password function
                    edit_password(client->username, new, client);
                } else {
                    // DEBUGGING
                    printf("[%s] Error: Flag type not found\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Flag type not found");
                    send(client_fd, response, strlen(response), 0);
                }

            } else if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *changed = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (changed == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing changed/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing changed or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if flag is valid
                if (strcmp(flag, "TO") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing flag)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Changed: %s, New: %s\n", client->username, flag, changed, new);

                // Call edit channel function
                edit_channel(changed, new, client);

            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *changed = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (changed == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing changed/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing changed or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if flag is valid
                if (strcmp(flag, "TO") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing flag)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Changed: %s, New: %s\n", client->username, flag, changed, new);

                // Call edit room function
                edit_room(changed, new, client);
                
            } else {
                // DEBUGGING
                printf("[%s] Error: Edit type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Edit type not found");
                send(client_fd, response, strlen(response), 0);
            
            }

 } else if (strcmp(command, "DEL") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid delete (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing delete type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Delete types
            if (strcmp(type, "CHAT") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing target)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, target);

                // Call delete chat function
                del_chat(atoi(target), client);

            } else if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing channel)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing channel)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Channel: %s\n", client->username, channel);

                // Call delete channel function
                delete_channel(channel, client);

            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *room = strtok(NULL, " ");

                // Check if command is valid
                if (room == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing room)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing room)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if ALL is used
                if (strcmp(room, "ALL") == 0){
                    // DEBUGGING
                    printf("[%s] Room: %s\n", client->username, room);

                    // Call delete all room function
                    delete_all_rooms(client);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Room: %s\n", client->username, room);

                // Call delete room function
                delete_room(room, client);

            } else {
                // DEBUGGING
                printf("[%s] Error: Delete type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Delete type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "JOIN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid target\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, target: %s\n", client->username, command, target);

            // Join channel if user is not in a channel
            if (strlen(client->channel) == 0){
                // Call join channel function
                join_channel(target, client);
            } else if (strlen(client->room) == 0){
                // Call join room function
                join_room(target, client);
            } else {
                // DEBUGGING
                printf("[%s] Error: User is already in a room\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: User is already in a channel");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "CHAT") == 0){
            // Parse data from client
            char *message = strtok(NULL, "\n");

            // Check if command is valid
            if (message == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid message (missing message)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing message)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // Check if message is correctly encased in quotes
            if (message[0] != '"' || message[strlen(message)-1] != '"'){
                // DEBUGGING
                printf("[%s] Error: Invalid message (missing quotes)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing quotes)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, message: %s\n", client->username, command, message);

            // Call send chat function
            send_chat(message, client);

 } else if (strcmp(command, "REMOVE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid remove (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing remove type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Check if type is USER
            if (strcmp(type, "USER") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid remove (missing target)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER * 2);
                    sprintf(response, "MSG,Error: Invalid command (missing remove target)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, type);

                // Call kick user function
                kick_user(target, client);
            } else {
                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, type);

                // Call remove user function
                remove_user(type, client);
            }

 } else if (strcmp(command, "BAN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid ban (missing target)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing ban target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, target);

            // Call ban user function
            ban_user(target, client);

 } else if (strcmp(command, "UNBAN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid unban (missing target)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER * 2);
                sprintf(response, "MSG,Error: Invalid command (missing unban target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, target);

            // Call unban user function
            unban_user(target, client);

        } else {
            // DEBUGGING
            printf("[%s] Error: Command not found\n", client->username);

            // Send response to client
            memset(response, 0, MAX_BUFFER * 2);
            sprintf(response, "MSG,Error: Command not found");
            send(client_fd, response, strlen(response), 0);
        }
    }
}
```
</details>

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256177652671315988/Screenshot_1972.png?ex=667fd21b&is=667e809b&hm=25593a966eeecd68228983e965fb2f37ce2f98d30189d51e3e05a4b3a53d0cc2&=&format=webp&quality=lossless&width=866&height=321)

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256177694295588894/Screenshot_1973.png?ex=667fd224&is=667e80a4&hm=05325dd64171f7a9694d15562a9c8fb1a3f37d304fe90cbd5b3eeaf0a3d8affe&=&format=webp&quality=lossless&width=911&height=399)

# Penggunaan DiscorIT

## Listing

### Channel
Proses listing channel di DiscorIT dilakukan dengan membaca file channels.csv. Pertama-tama, file channels.csv dibuka untuk membaca daftar channel yang tersedia. Jika file tidak dapat dibuka, sistem akan mengirimkan pesan kesalahan kepada client. Selanjutnya, sistem melakukan iterasi melalui setiap baris dalam file yang berisi ID dan nama channel. Setiap baris dipisahkan menggunakan fscanf, yang mengidentifikasi ID dan nama channel. Jika ada channel yang ditemukan, nama-nama channel tersebut akan digabungkan ke dalam satu string respon yang kemudian dikirimkan ke client. Jika tidak ada channel yang ditemukan, sistem akan mengirimkan pesan bahwa tidak ada channel yang ditemukan. Setelah iterasi selesai, file ditutup dan respon dikirimkan ke client.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//===============//
// LIST CHANNELS //
//===============//

void list_channel(client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(channels_csv, "r");

    // Prepare response
    char response[MAX_BUFFER * 2];
    sprintf(response, "MSG,");
    int channels_found = 0;

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and channel
    int id = 1; char channel[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, channel) == 2) {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] id: %d, channel: %s\n", client->username, id, channel);

        // Increment channels found
        channels_found++;

        // Concatenate response
        if (id > 1) strcat(response, " ");
        strcat(response, channel);
    }

    // Close file
    fclose(file);

    // Send response to client
    if (channels_found == 0) {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] No channels found\n", client->username);

        sprintf(response, "MSG,No channels found");
        send(client_fd, response, strlen(response), 0);
    } else {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] Channels found: %d\n", client->username, channels_found);

        send(client_fd, response, strlen(response), 0);
    }

    return;
}
```
</details>

### Room
Proses listing room dilakukan dengan membaca semua folder dalam sebuah channel kecuali folder yang bernama "admin". Pertama, sistem memeriksa apakah user sudah berada dalam suatu channel. Jika belum, sistem mengirimkan pesan kesalahan kepada client. Kemudian, sistem menyiapkan path ke channel yang sedang digunakan oleh user dan membuka direktori channel tersebut. Jika direktori tidak bisa dibuka, sistem kembali mengirimkan pesan kesalahan. Setelah berhasil membuka direktori, sistem melakukan iterasi melalui setiap entry di dalam direktori tersebut, melewatkan entry yang bernama "." dan "..". Sistem juga melewatkan entry dengan nama "admin". Untuk setiap entry yang merupakan direktori, sistem menambahkan nama direktori tersebut ke dalam string respon. Setelah iterasi selesai, direktori ditutup dan sistem mengirimkan string respon yang berisi daftar nama-nama room kepada client. Jika tidak ada room yang ditemukan, sistem mengirimkan pesan bahwa tidak ada room yang ditemukan.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//============//
// LIST ROOMS //
//============//

void list_room(client_data *client) {
    int client_fd = client->socket_fd;

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][LIST ROOM] Error: User is not in a channel\n", client->username);

        // Prepare response
        char response[MAX_BUFFER * 2];
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare channel path
    char path_channel[MAX_BUFFER * 2];
    sprintf(path_channel, "%s/%s", cwd, client->channel);

    // Prepare response
    char response[MAX_BUFFER * 2];
    sprintf(response, "MSG,");
    int rooms_found = 0;

    // Open channel directory
    DIR *dir = opendir(path_channel);

    // Fail if directory cannot be opened
    if (dir == NULL) {
        // DEBUGGING
        printf("[%s][LIST ROOM] Error: Unable to open directory\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open directory");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // DEBUGGING
        printf("[%s][LIST ROOM] Room: %s\n", client->username, entry->d_name);

        // Skip admin directory
        if (strcmp(entry->d_name, "admin") == 0) continue;

        // Prepare entry path
        char entry_path[MAX_BUFFER * 3];
        sprintf(entry_path, "%s/%s", path_channel, entry->d_name);

        // Check if entry is a directory to be listed
        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) continue;
        if (S_ISDIR(statbuf.st_mode)) {
            // Increment rooms found
            rooms_found++;

            // Concatenate response
            if (rooms_found > 1) strcat(response, " ");
            strcat(response, entry->d_name);
        }   
    }

    // Close directory
    closedir(dir);

    // DEBUGGING
    printf("[%s][LIST ROOM] Rooms found: %d\n", client->username, rooms_found);

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        send(client_fd, response, strlen(response), 0);
    }

    return;
}
```
</details>

### User
Proses listing user dilakukan dengan membaca file auth.csv dan hanya bisa dijalankan oleh user dengan peran "ROOT". Pertama, sistem memeriksa apakah user yang menjalankan perintah memiliki peran "ROOT". Jika tidak, sistem mengirimkan pesan kesalahan kepada client. Jika user adalah "ROOT", sistem membuka file auth.csv yang menyimpan data pengguna. Jika file tidak bisa dibuka, sistem mengirimkan pesan kesalahan. Setelah file berhasil dibuka, sistem melakukan iterasi melalui setiap baris dalam file, mengambil nama pengguna dan menambahkannya ke dalam string respon. Setelah iterasi selesai, file ditutup dan sistem mengirimkan string respon yang berisi daftar nama pengguna kepada client.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//============//
// LIST USERS //
//============//

void list_user(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER * 2];
    sprintf(response, "MSG,");

    // Check if user is not root
    if (strcmp(client->role, "ROOT") != 0) {
        // DEBUGGING
        printf("[%s][LIST USER] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open users file
    FILE *file = fopen(users_csv, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][LIST USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username
    char namecheck[MAX_BUFFER];
    while (fscanf(file, "%*d,%[^,],%*[^,],%*s)", namecheck) == 1) {
        // DEBUGGING
        printf("[%s][LIST USER] User: %s\n", client->username, namecheck);

        // Concatenate response
        if (strcmp(response, "MSG,") != 0) strcat(response, " ");
        strcat(response, namecheck);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256182781642543124/Screenshot_1974.png?ex=667fd6e1&is=667e8561&hm=3ecc3291947e450a2137ac9b486d5a092ade187780ed68f0f2933939463a6f91&=&format=webp&quality=lossless&width=785&height=380)

## Joining

### Channel

Proses bergabung ke channel pada DiscorIT diawali dengan pengecekan apakah channel yang ingin dimasuki user ada dengan memanggil fungsi check_channel. Jika channel tidak ada, sistem mengirim pesan kesalahan. Setelah memastikan channel ada, sistem membuka file auth.csv yang berada di folder admin dari channel tersebut. File ini digunakan untuk mengecek status user. Pertama, sistem membaca file auth.csv dan memeriksa apakah user sudah terdaftar. Jika user terdaftar dan berstatus "BANNED", sistem mengirim pesan kesalahan dan tidak mengizinkan user masuk. Jika user terdaftar dan tidak berstatus "BANNED", sistem memperbolehkan user masuk ke channel dan mencatat aksi tersebut di log. Jika user tidak terdaftar di auth.csv dan memiliki peran "ROOT", sistem langsung menambahkan user sebagai "ROOT" di channel tersebut dan memperbarui file auth.csv. Namun, jika user tidak terdaftar dan bukan "ROOT", sistem akan meminta verifikasi kunci (key) melalui fungsi verify_key. Jika verifikasi berhasil, user ditambahkan ke channel dengan status "USER" dan aksi ini juga dicatat di log. Monitor tidak boleh memasukkan kunci verifikasi.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c

//==============//
// JOIN CHANNEL //
//==============//

void join_channel(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER * 2];
    sprintf(response, "MSG,");

    // Check if channel exists
    if (check_channel(channel, client) != -2) {
        // DEBUGGING
        printf("[%s][JOIN CHANNEL] Error: Channel does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Channel does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open auth of current channel
    char path_auth[MAX_BUFFER * 2];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);
    FILE *file = fopen(path_auth, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][JOIN CHANNEL] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    char username[MAX_BUFFER]; char role[8];
    while (fscanf(file, "%*d,%[^,],%s", username, role) == 2) {
        // DEBUGGING
        printf("[%s][JOIN CHANNEL] username: %s, role: %s\n", client->username, username, role);

        // If there is a username match or user is root
        if (strcmp(client->username, username) == 0) {
            // Check if user is banned
            if (strcmp(role, "BANNED") == 0) {
                // DEBUGGING
                printf("[%s][JOIN CHANNEL] Error: User is banned from %s\n", client->username, channel);

                // Close file
                fclose(file);

                // Send response to client
                sprintf(response, "MSG,Error: User is banned from channel");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("[%s][JOIN CHANNEL] Success: User joined %s\n", client->username, channel);

            // Close file
            fclose(file);

            // Update client channel
            strcpy(client->channel, channel);

            // Write to log
            char message[MAX_BUFFER * 2];
            sprintf(message, "%s joined channel \"%s\"\n", client->username, channel);
            write_log(channel, message);

            // Send response to client
            if (strcmp(client->role, "ROOT") == 0)
                sprintf(response, "CHANNEL,Joined channel %s as ROOT,%s", channel, channel);
            else
                sprintf(response, "CHANNEL,Joined channel %s,%s", channel, channel);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Add user to channel if user is root and not listed
    if (strcmp(client->role, "ROOT") == 0) {
        // Open auth of current channel and add user
        fprintf(file, "%d,%s,ROOT\n", client->id, client->username);

        // DEBUGGING
        printf("[%s][JOIN CHANNEL] Success: Root joined %s\n", client->username, channel);

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s joined channel \"%s\" as ROOT\n", client->username, channel);
        write_log(channel, message);

        // Send response to client
        sprintf(response, "CHANNEL,Joined channel %s as ROOT,%s", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Verify key if user is not listed
    if (verify_key(channel, client) == 1) {
        // Open auth of current channel and add user
        fprintf(file, "%d,%s,USER\n", client->id, client->username);

        // DEBUGGING
        printf("[%s][JOIN CHANNEL] Success: User joined %s\n", client->username, channel);

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s verified for channel \"%s\"\n", client->username, channel);
        write_log(channel, message);

        // Send response to client
        sprintf(response, "CHANNEL,Joined channel %s,%s", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Close file
    fclose(file);

    // Send response to client
    sprintf(response, "MSG,Error: Key verification failure");
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### Room
Proses join room dimulai dengan pengecekan apakah user mencoba masuk ke room dengan nama "admin", yang tidak diizinkan, dan akan menghasilkan pesan kesalahan. Selanjutnya, sistem memeriksa apakah user sudah berada dalam suatu channel; jika belum, maka akan mengirim pesan kesalahan. Setelah memastikan user berada di dalam channel, sistem memeriksa keberadaan room yang ingin dimasuki user dengan mengecek direktori yang sesuai. Jika room tidak ada, sistem mengirim pesan kesalahan. Jika room ada dan user belum berada dalam room lain, sistem akan mengizinkan user untuk bergabung dengan room tersebut. Nama room akan disimpan dalam atribut room pada client_data, dan tindakan ini akan dicatat dalam log channel. Sebagai respon, sistem akan mengirim pesan ke user bahwa user telah berhasil bergabung dengan room tersebut. Jika user sudah berada dalam room lain, sistem akan mengirim pesan kesalahan bahwa user sudah berada dalam room dan tidak dapat bergabung dengan room baru tanpa keluar dari room sebelumnya.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//===========//
// JOIN ROOM //
//===========//

void join_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to join admin
    if (strcmp(room, "admin") == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: Invalid room name\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if room exists
    char path_room[MAX_BUFFER * 2];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Join room if user is not in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Success: User joined %s\n", client->username, room);

        // Update client room
        strcpy(client->room, room);

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s joined room \"%s\"\n", client->username, room);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "ROOM,Joined room %s,%s", room, room);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][JOIN ROOM] Error: User is already in a room\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Error: User is already in a room");
    send(client_fd, response, strlen(response), 0);
}
```
</details>

## Chatting

### Get Timestamp
Fungsi `get_timestamp()` adalah fungsi pendukung yang esensial dalam sistem chat karena menghasilkan timestamp yang digunakan untuk merekam waktu pengiriman chat dalam room chat. Saat digunakan dalam konteks aplikasi chat, timestamp ini bertindak sebagai penanda waktu yang menunjukkan kapan chat dikirim. Prosesnya dimulai dengan memanggil fungsi `time(&rawtime)`, yang mengambil waktu saat ini dari sistem dan menyimpannya dalam variabel `rawtime` bertipe `time_t`. Selanjutnya, fungsi `localtime(&rawtime)` mengkonversi waktu dalam `rawtime` ke dalam struktur `struct tm`, yang berisi informasi terperinci tentang tahun, bulan, hari, jam, menit, dan detik dalam zona waktu lokal user.

Setelah memperoleh struktur waktu melalui `localtime`, fungsi `strftime(timestamp, MAX_BUFFER, "%d/%m/%Y %H:%M:%S", timeinfo)` digunakan untuk memformat informasi waktu tersebut ke dalam string `timestamp` dengan format "dd/mm/yyyy HH:MM:SS". String `timestamp` yang dihasilkan kemudian digunakan untuk mencatat waktu pengiriman setiap chat yang dikirimkan dalam room chat. Ini penting karena memungkinkan user untuk melihat urutan chat dan waktu pengirimannya, serta memungkinkan administrator atau user lain untuk memantau aktivitas chat secara kronologis. Oleh karena itu, fungsi `get_timestamp()` tidak hanya menyediakan data waktu tetapi juga mendukung fungsionalitas sistem yang mengandalkan urutan waktu untuk mengelola dan mempresentasikan dialog dalam aplikasi chat.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
    // Get timestamp
    char *timestamp = get_timestamp();
    fprintf(file, "%s,%d,%s,%s\n", timestamp, id+1, client->username, message);
    ```
</details>

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//===============//
// GET TIMESTAMP //
//===============//

char* get_timestamp() {
    time_t rawtime;
    struct tm *timeinfo;
    char *timestamp = (char *)malloc(sizeof(char) * MAX_BUFFER);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, MAX_BUFFER, "%d/%m/%Y %H:%M:%S", timeinfo);

    return timestamp;
}
```
</details>

### Send Chat
Proses send chat dimulai dengan pengecekan apakah user berada dalam sebuah room. Jika user tidak berada dalam room, sistem akan mengirim pesan kesalahan. Jika user berada dalam room, sistem mempersiapkan path file chat.csv di dalam direktori room tersebut. Sistem kemudian membuka file chat.csv untuk menambahkan pesan baru. Jika file tidak bisa dibuka, sistem mengirim pesan kesalahan. Namun, jika file berhasil dibuka, sistem membaca file untuk mendapatkan ID terakhir dari pesan yang ada. Setelah itu, sistem mengambil timestamp saat ini dan menambahkan pesan baru dengan format timestamp, ID, username, message ke file chat.csv. Setelah pesan berhasil ditambahkan, file ditutup, dan sistem mengirim pesan sukses ke user yang berisi timestamp, ID, dan username dari pesan yang baru dikirim.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// SEND CHAT DATA //
//================//

void send_chat(char *message, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_CHAT];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][SEND CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER * 2];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);

    // Open chat file
    FILE *file = fopen(path_chat, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][SEND CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through csv to get ID
    int id = 0;
    while (fscanf(file, "%*[^,],%d,%*[^,],%*[^\n]", &id) == 1) {
        // DEBUGGING
        printf("[%s][SEND CHAT] id: %d\n", client->username, id);
    }

    // Get timestamp
    char *timestamp = get_timestamp();
    fprintf(file, "%s,%d,%s,%s\n", timestamp, id+1, client->username, message);

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][SEND CHAT] Success: Chat sent\n", client->username);

    // Send response to client
    sprintf(response, "MSG,[%s][%d][%s][SENT]", timestamp, id+1, client->username);
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### See Chat
Proses see chat dimulai dengan pengecekan apakah user berada dalam sebuah room. Jika user tidak berada dalam room, sistem akan mengirim pesan kesalahan. Jika user berada dalam room, sistem mempersiapkan path file chat.csv di dalam direktori room tersebut dan membuka file tersebut untuk membaca pesan-pesan yang ada. Jika file tidak bisa dibuka, sistem mengirim pesan kesalahan. Namun, jika file berhasil dibuka, sistem akan memeriksa apakah file chat kosong. Jika kosong, sistem mengirim pesan kesalahan bahwa chat kosong. Jika file tidak kosong, sistem membaca isi file chat satu per satu dan menggabungkan pesan-pesan tersebut ke dalam satu respons yang akan dikirim ke user. Jika ukuran respons terlalu besar, proses penggabungan pesan akan dihentikan untuk menghindari pengiriman data yang terlalu besar. Setelah semua pesan yang bisa ditampilkan digabungkan, file ditutup, dan sistem mengirimkan respons berisi pesan-pesan chat ke user.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//===============//
// SEE CHAT DATA //
//===============//

void see_chat(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare larger response for chat
    char chat[MAX_BUFFER+256];
    char response[MAX_CHAT];
    sprintf(response, "MSG,");

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER * 2];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);

    // Open chat file
    FILE *file = fopen(path_chat, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if chat is empty
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: Chat is empty\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Chat is empty");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    fseek(file, 0, SEEK_SET);

    // Loop through csv to get messages
    int id; char timestamp[20], username[100], message[MAX_BUFFER];
    while (fscanf(file, " %[^,],%d,%[^,],%[^\n]",
           timestamp, &id, username, message) == 4) {
        // DEBUGGING
        printf("[%s][SEE CHAT] [%s][%d][%s] %s\n", client->username, timestamp, id, username, message);

        // Prepare chat send
        snprintf(chat, sizeof(chat), "[%s][%d][%s] %s", timestamp, id, username, message);

        // Break if response is too large
        if (strlen(response) + strlen(chat) + 2 > sizeof(response)) break;

        // Concatenate response with chat
        if (strlen(response) > 4) strcat(response, "\n");
        strcat(response, chat);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### Edit Chat
Dalam fungsi `edit_chat`, proses edit dilakukan dengan cara membaca setiap baris dari file `chat.csv` dan menyalinnya ke sebuah file sementara (`temp_file`) kecuali baris yang sesuai dengan ID yang ingin diubah. Pada tahap ini, sistem memeriksa peran user (`client->role`). Jika user memiliki peran `ROOT` atau `ADMIN`, mereka diizinkan untuk mengedit semua pesan dalam room tersebut. Namun, jika user memiliki peran `USER`, sistem memastikan bahwa mereka hanya dapat mengedit pesan yang mereka tulis sendiri dengan memeriksa username yang terkait dengan pesan. Proses ini memastikan bahwa hanya pesan yang sesuai dengan aturan perizinan yang diizinkan untuk diedit, sesuai dengan kebutuhan dan keamanan sistem. Setelah edit selesai, file sementara digunakan untuk mengganti file `chat.csv` asli, dan informasi edit dicatat dalam log sistem sebelum memberikan respons sukses kepada user.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// EDIT CHAT DATA //
//================//

void edit_chat(int edit, char *message, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER * 2];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);
    FILE *file = fopen(path_chat, "r");

    // Prepare temp path
    char path_temp[MAX_BUFFER * 2];
    sprintf(path_temp, "%s/%s/%s/.chat_temp.csv", cwd, client->channel, client->room);
    FILE *file_temp = fopen(path_temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][EDIT CHAT] id: %d, message: %s\n", client->username, edit, message);

    // Loop until id matches
    int id; char timestamp[20], username[100], message_old[MAX_BUFFER];
    char buffer[MAX_CHAT];
    int found = 0;
    while (fgets(buffer, MAX_BUFFER, file) != NULL) {
        // Get data from buffer
        sscanf(buffer, "%[^,],%d,%[^,],%[^\n]", timestamp, &id, username, message_old);

        // DEBUGGING
        printf("[%s][EDIT CHAT] id: %d, username: %s, message: %s\n", client->username, id, username, message_old);

        // Edit if id matches
        if (id == edit){
            // Check if user is not admin/root
            if (strcmp(client->role, "USER") == 0)
            // Check if user is not the author
            if (strcmp(client->username, username) != 0) {
                // DEBUGGING
                printf("[%s][EDIT CHAT] Error: User is not admin/root\n", client->username);

                // Close files
                fclose(file);
                fclose(file_temp);

                // Unlink temp file
                remove(path_temp);

                // Send response to client
                sprintf(response, "MSG,Error: User is not admin/root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            found = 1;
            fprintf(file_temp, "%s,%d,%s,%s\n", timestamp, id, username, message);
            continue;
        }

        // Write to temp file
        fprintf(file_temp, "%s", buffer);
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Check if id is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: ID not found\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: ID not found");
        send(client_fd, response, strlen(response), 0);
        return;
    } 

    // DEBUGGING
    printf("[%s][EDIT CHAT] Success: ID edited\n", client->username);

    // Remove original file
    remove(path_chat);

    // Rename temp file
    rename(path_temp, path_chat);

    // Write to log
    char log[MAX_BUFFER * 2];
    sprintf(log, "%s edited chat id %d in \"%s\": %s\n", client->username, edit, client->room, message);
    write_log(client->channel, log);

    // Send response to client
    sprintf(response, "MSG,Success: ID edited");
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### Delete Chat
Fungsi `del_chat` digunakan untuk menghapus chat dalam sebuah room chat berdasarkan ID tertentu. Pertama, fungsi memeriksa apakah user sedang berada di room chat tersebut. Jika tidak, fungsi akan memberitahu user bahwa mereka harus berada di room chat untuk melakukan penghapusan. Selanjutnya, fungsi membuka file tempat chat disimpan dan membuat file sementara untuk menyimpan hasil edit. Kemudian, fungsi membaca setiap chat dalam file chat. Ketika menemukan chat dengan ID yang sesuai dengan yang diminta untuk dihapus, fungsi memeriksa izin user: jika user adalah admin atau root, mereka dapat menghapus chat apa pun; jika hanya user biasa, mereka hanya bisa menghapus chat yang mereka tulis sendiri. Setelah menemukan chat yang sesuai untuk dihapus, chat tersebut tidak disalin ke file sementara, sehingga dihapus dari file asli. Jika ID yang diminta untuk dihapus tidak ditemukan, fungsi akan memberitahu user bahwa ID tersebut tidak ada dalam room chat. Setelah menghapus chat, file asli chat diperbarui dengan file sementara yang berisi perubahan, dan kegiatan penghapusan dicatat dalam log sistem. Akhirnya, user diberi tahu bahwa penghapusan berhasil dilakukan.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// DELETE CHAT ID //
//================//

void del_chat(int target, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER * 2];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);
    FILE *file = fopen(path_chat, "r");

    // Prepare temp path
    char path_temp[MAX_BUFFER * 2];
    sprintf(path_temp, "%s/%s/%s/.chat_temp.csv", cwd, client->channel, client->room);
    FILE *file_temp = fopen(path_temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DEL CHAT] id: %d\n", client->username, target);

    // Loop until id matches
    int id; char username[MAX_BUFFER], message[MAX_BUFFER];
    char buffer[MAX_CHAT];
    int found = 0;
    while (fgets(buffer, MAX_BUFFER, file) != NULL) {
        // Get data from buffer
        sscanf(buffer, "%*[^,],%d,%[^,],%[^\n]", &id, username, message);

        // DEBUGGING
        printf("[%s][DEL CHAT] id: %d, username: %s\n", client->username, id, username);

        // Skip if id matches
        if (id == target){
            // Check if user is not admin/root
            if (strcmp(client->role, "USER") == 0)
            // Check if user is not the author
            if (strcmp(client->username, username) != 0) {
                // DEBUGGING
                printf("[%s][DEL CHAT] Error: User is not admin/root\n", client->username);

                // Close files
                fclose(file);
                fclose(file_temp);

                // Unlink temp file
                remove(path_temp);

                // Send response to client
                sprintf(response, "MSG,Error: User is not admin/root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            found = 1;
            continue;
        }

        // Write to temp file
        fprintf(file_temp, "%s", buffer);
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Check if id is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: ID not found\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: ID not found");
        send(client_fd, response, strlen(response), 0);
        return;
    } 

    // DEBUGGING
    printf("[%s][DEL CHAT] Success: ID deleted\n", client->username);

    // Remove original file
    remove(path_chat);

    // Rename temp file
    rename(path_temp, path_chat);

    // Write to log
    char log[MAX_BUFFER * 3];
    sprintf(log, "%s deleted chat id %d in \"%s\": \"%s\"\n", client->username, target, client->room, message);
    write_log(client->channel, log);

    // Send response to client
    sprintf(response, "MSG,Success: ID deleted");
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256190862434504776/IMG-20240622-WA0058.jpg?ex=667fde68&is=667e8ce8&hm=9d7fc1e2b65922b61ae06f06ab0c4bf6ce898a4062869570b98cc52b9bb3d95f&=&format=webp&width=1166&height=656)

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256190862891814953/IMG-20240623-WA0003.jpg?ex=667fde68&is=667e8ce8&hm=b5d2d8bacdeefd532ca61ba8117eda09e728348ffa695dd9e8483e34804251fc&=&format=webp&width=600&height=583)

## Root Actions

### Check User
Fungsi `check_user` digunakan untuk mengecek apakah sebuah nama user (username) sudah terdaftar dalam sebuah file. Fungsi ini membuka file yang berisi daftar user, memeriksa setiap username dalam file tersebut, dan memberitahu jika username yang dicari ada atau tidak. Hal ini penting untuk memastikan bahwa hanya user yang terdaftar yang bisa menggunakan fitur-fitur tertentu dalam aplikasi, menjaga keamanan dan integritas data.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//======================//
// CHECK USER EXISTENCE //
//======================//

int check_user(client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(users_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CHECK USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    // Loop through id and username
    int id; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("[%s][CHECK USER] id: %d, name: %s\n", client->username, id, namecheck);

        // Return 1 if user exists
        if (strcmp(namecheck, client->username) == 0) {
            // DEBUGGING
            printf("[%s][CHECK USER] Success: User exists\n", client->username);

            // Close file
            fclose(file);
            return 1;
        }
    }

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][CHECK USER] Error: User does not exist\n", client->username);

    // Return 0 if user does not exist
    return 0;
}
```
</details>

### Editing User (Username)
Untuk mengedit nama pengguna (username) dalam sistem, fungsi `edit_username` maka pertama, fungsi memeriksa apakah user yang meminta edit adalah admin atau root; jika bukan, maka permintaan akan ditolak. Kemudian, fungsi membuka file yang berisi daftar user untuk memeriksa setiap entri. Selama proses ini, fungsi membandingkan setiap username dengan nama yang ingin diubah. Jika username ditemukan, fungsi menulis entri baru dengan username yang baru ke dalam file sementara, sementara entri yang lain tetap menggunakan username yang lama. Setelah selesai, file asli dihapus dan file sementara diganti namanya menjadi file utama, sehingga perubahan tersimpan. Jika username yang dicari tidak ditemukan dalam file, fungsi akan mengirimkan pesan kesalahan ke user. Setelah proses utama selesai, fungsi memanggil `edit_username_auth` untuk melakukan pembaruan username di seluruh channel yang tersimpan dalam file `auth.csv`. Proses ini penting untuk memastikan konsistensi username di seluruh sistem, memungkinkan user untuk terus menggunakan aplikasi dengan nama yang baru.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// EDIT USER NAME //
//================//

void edit_username(char *username, char *newusername, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT USERNAME] username: %s, newusername: %s\n", client->username, username, newusername);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME] Success: Username found\n", client->username);

            // Write new username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, newusername, passcheck, role);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT USERNAME] Success: Main username edited\n", client->username);

    // Call edit_username_auth
    edit_username_auth(username, newusername, client);
    return;
}
```
</details>

#### Bonus Case 1
Ketika root melakukan pengeditan username dirinya sendiri, proses ini harus secara otomatis mengupdate data client yang tersimpan di server. Selain itu, program client harus menerima pembaruan ini untuk memperbarui tampilan username yang baru. Hal ini memastikan bahwa semua data dan tampilan username tetap konsisten di seluruh sistem.
Pada bagian kode edit_username, kita perlu menambahkan logika untuk mengupdate data client di server dan mengirimkan pesan kepada client untuk memperbarui nama pengguna.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// EDIT USER NAME //
//================//

void edit_username(char *username, char *newusername, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT USERNAME] username: %s, newusername: %s\n", client->username, username, newusername);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME] Success: Username found\n", client->username);

            // Write new username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, newusername, passcheck, role);
            found = 1;
            // Update client data
            strcpy(client->username, newusername);
            // Inform the client to update their display name
            char msg[MAX_BUFFER];
            sprintf(msg, "MSG,Your username has been changed to %s", newusername);
            send(client_fd, msg, strlen(msg), 0);
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT USERNAME] Success: Main username edited\n", client->username);

    // Call edit_username_auth
    edit_username_auth(username, newusername, client);
    return;
}
```
</details>
Dan Fungsi edit_username_auth mengupdate data nama pengguna di seluruh channel yang terdaftar di auth.csv.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void edit_username_auth(char *username, char *newusername, client_data *client) {
    // Prepare response
    char response[MAX_BUFFER];

    // Open auth file
    FILE *file = fopen(auth_csv, "r");
    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_auth.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME AUTH] Error: Unable to open auth file\n", client->username);
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client->socket_fd, response, strlen(response), 0);
        return;
    }

    int id; char channel[MAX_BUFFER], user[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%s", &id, channel, user) == 3) {
        // Check for username to update
        if (strcmp(user, username) == 0) {
            fprintf(temp, "%d,%s,%s\n", id, channel, newusername);
        } else {
            fprintf(temp, "%d,%s,%s\n", id, channel, user);
        }
    }

    fclose(file);
    fclose(temp);

    remove(auth_csv);
    rename(temp_csv, auth_csv);

    // Update all channels
    update_channels(username, newusername);

    // Notify all clients
    sprintf(response, "MSG,Your username has been changed to %s", newusername);
    send(client->socket_fd, response, strlen(response), 0);
}
```
</details>

#### Bonus Case 2

Ketika seorang user berada dalam sebuah channel, dan seorang root (orang lain) melakukan edit pada nama user, untuk menghindari kerusakan pada sistem, user akan otomatis keluar dari DiscorIT tanpa diban atau keluar dari user terdaftar pada channel. Contoh:

* User join Channel
* Root rename User
* Command berikutnya dari User akan ditolak
* User akan keluar dari DiscorIT secara aman

### Editing User (Password)
Fungsi `edit_password` digunakan untuk mengizinkan admin atau root untuk mengubah kata sandi pengguna yang terdaftar. Pertama, fungsi memeriksa izin pengguna yang meminta perubahan ini. Jika izinnya tidak sesuai, fungsi akan mengirim pesan kesalahan kembali kepada pengguna. Selanjutnya, fungsi membuka file yang berisi data pengguna utama (`users_csv`) untuk mencari username yang diminta. Jika username ditemukan, fungsi akan mengganti password lama dengan yang baru, yang di-hash menggunakan metode kriptografi tertentu untuk keamanan. Data pengguna yang diperbarui disimpan sementara dalam file, kemudian file asli diperbarui dengan yang baru. Jika username tidak ditemukan, operasi akan gagal dan pengguna akan diberitahu bahwa username yang dimaksud tidak terdaftar dalam sistem.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// EDIT USER PASS //
//================//

void edit_password(char *username, char *newpassword, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] username: %s, newpassword: %s\n", client->username, username, newpassword);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r+");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT PASSWORD] Success: Username found\n", client->username);

            // Hash new password
            char hash[MAX_BUFFER];
            strcpy(hash,crypt(newpassword, HASHCODE));

            // Write new password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, hash, role);
            found = 1;
        } else {
            // Write old password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] Success: Password edited\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Password edited");
    send(client_fd, response, strlen(response), 0);
    return;    
}
```
</details>

### Remove User Account
Fungsi `remove_user` digunakan untuk menghapus pengguna yang terdaftar. Prosesnya mirip dengan fungsi `edit_username` namun dengan perbedaan bahwa pengguna dihapus dari sistem. Pertama, fungsi memeriksa apakah pengguna yang meminta penghapusan adalah root atau admin, dan bukan mencoba menghapus dirinya sendiri. Kemudian, fungsi membuka file yang berisi daftar pengguna (`users_csv`) dan mencari pengguna yang akan dihapus. Jika pengguna ditemukan dan bukan root, data pengguna tersebut tidak disalin ke file sementara, sehingga dihapus dari sistem. Jika pengguna yang dicari tidak ditemukan, proses gagal dan pengguna yang meminta penghapusan diberitahu. Setelah proses penghapusan dari file utama selesai, fungsi memanggil `del_username_auth` untuk menghapus pengguna tersebut dari semua file otorisasi (`auth.csv`) yang ada di setiap channel, memastikan bahwa pengguna benar-benar dihapus dari seluruh sistem.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//=============//
// REMOVE USER //
//=============//

void remove_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][REMOVE USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to remove self
    if (strcmp(client->username, username) == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is trying to remove self\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is trying to remove self");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][REMOVE USER] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // Check if role is not root
            if (strcmp(role, "ROOT") == 0) {
                // DEBUGGING
                printf("[%s][REMOVE USER] Error: User is root\n", client->username);

                // Close files
                fclose(file);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("[%s][REMOVE USER] Success: Username found\n", client->username);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][REMOVE USER] Success: User removed\n", client->username);

    // Call del_username_auth
    del_username_auth(username, client);
    return;
}
```
</details>

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256190055261802557/IMG-20240623-WA0010.jpg?ex=667fdda8&is=667e8c28&hm=55f9ca1858d57650688754863a3874c3c8c72c1c359be5faba01c262dae8be79&=&format=webp&width=653&height=656)

![alt text](https://media.discordapp.net/attachments/1256171847284953088/1256190055563661403/IMG-20240623-WA0011.jpg?ex=667fdda8&is=667e8c28&hm=4a2b97ca5e42b49230f7cb180b20c2e9a9050f3521f1d32ec59d649716f8417b&=&format=webp&width=967&height=656)

#### Bonus Case 1

Karena ada fitur "Remove User from Channel" yang menggunakan awalan "REMOVE", tetapi dilanjut oleh tag "USER", maka sistem melarang untuk pengguna melakukan register akun atas nama "USER" untuk menghindari konflik antara "Remove User Account" dan "Remove User from Channel".

#### Bonus Case 2

Ketika seorang user berada dalam sebuah channel, dan seorang root (orang lain) melakukan remove pada user, untuk menghindari kerusakan pada sistem, user akan otomatis keluar dari DiscorIT. Contoh:

* User join Channel
* Root rename User
* Command berikutnya dari User akan ditolak
* User akan keluar dari DiscorIT

## Admin Actions

### Fungsi Pendukung
Pertama, fungsi make_directory digunakan untuk membuat direktori baru di sistem. Ketika fungsi ini dipanggil, ia mencoba membuat direktori dengan jalur yang diberikan dan memberikan izin penuh (baca, tulis, dan eksekusi) kepada semua pengguna. Jika proses pembuatan direktori gagal dan penyebab kegagalannya bukan karena direktori sudah ada, fungsi ini akan menampilkan pesan error dan berhenti.

Selanjutnya, ada fungsi rename_directory yang bertugas mengganti nama direktori. Fungsi ini memulai dengan membuat proses anak menggunakan fork. Jika proses fork gagal, fungsi ini akan menampilkan pesan error dan berhenti. Jika berhasil, proses anak akan menjalankan perintah mv untuk mengganti nama direktori sesuai dengan jalur baru yang diberikan. Sementara itu, proses induk akan menunggu hingga proses anak selesai bekerja.

Terakhir, fungsi remove_directory digunakan untuk menghapus direktori beserta isinya. Sama seperti fungsi rename_directory, fungsi ini juga membuat proses anak menggunakan fork. Jika fork gagal, fungsi ini akan menampilkan pesan error dan berhenti. Proses anak kemudian menjalankan perintah rm -rf untuk menghapus direktori dan semua isinya. Proses induk akan menunggu hingga proses anak selesai sebelum kembali ke tugas lainnya.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// MAKE DIRECTORY //
//================//

void make_directory(char *path) {
    // Create directory
    if (mkdir(path, 0777) == -1)
    // Fail if directory cannot be created
    if (errno != EEXIST) {
        printf("[MKDIR] Error: Unable to create directory\n");
        return;
    }
}

//==================//
// RENAME DIRECTORY //
//==================//

void rename_directory(char *path, char *newpath) {
    // Create fork
    int pid = fork();
    // Return if fork fails
    if (pid < 0) {
        printf("[RENAME] Error: Unable to fork\n");
        return;
    }

    // Execute mv
    if (pid == 0) {
        execlp("mv", "mv", path, newpath, NULL);
    }

    // Wait for child process
    wait(NULL);
    return;
}

//==================//
// REMOVE DIRECTORY //
//==================//

void remove_directory(char *path) {
    // Create fork
    int pid = fork();
    // Return if fork fails
    if (pid < 0) {
        printf("[RMDIR] Error: Unable to fork\n");
        return;
    }

    // Execute rm -rf
    if (pid == 0) {
        execlp("rm", "rm", "-rf", path, NULL);
    }

    // Wait for child process
    wait(NULL);
    return;
}
```
</details>

### Check Channel Permission
Fungsi `check_channel_perms` digunakan untuk memeriksa apakah seorang user memiliki izin khusus (seperti admin atau root) di dalam sebuah channel tertentu. Pertama, fungsi ini membuka file `channels_csv` untuk membaca daftar channel yang tersedia. Jika file tersebut tidak bisa dibuka, fungsi ini akan mencatat pesan error dan mengembalikan nilai -1. Selanjutnya, fungsi ini akan membaca setiap nama channel dalam file tersebut dan membandingkannya dengan nama channel yang dituju (`target`). Jika nama channel cocok, fungsi akan membuka file `auth.csv` di dalam direktori channel tersebut untuk memeriksa izin user. Di dalam file `auth.csv`, fungsi ini mencari nama user dan perannya. Jika menemukan bahwa user adalah admin atau root, fungsi ini akan menutup semua file yang terbuka dan mengembalikan nilai 1, menunjukkan bahwa user memiliki izin khusus. Jika tidak menemukan izin khusus untuk user tersebut, fungsi akan menutup semua file yang terbuka dan mengembalikan nilai 0, menunjukkan bahwa user tidak memiliki izin khusus.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//=====================//
// CHECK CHANNEL PERMS //
//=====================//

int check_channel_perms(char *target, client_data *client) {
    int client_fd = client->socket_fd;

    // Check channel permissions
    FILE *file_channel = fopen(channels_csv, "r");

    // Fail if file cannot be opened
    if (file_channel == NULL) {
        // DEBUGGING
        printf("[%s][CHECK PERMS] Error: Unable to open file\n", client->username);

        // Return -1 if file cannot be opened
        return -1;
    }

    // Loop through channel name to find permissions
    char channel[MAX_BUFFER];
    while (fscanf(file_channel, "%*d,%[^,],%*s", channel) == 1) {
        // DEBUGGING
        printf("[%s][CHECK PERMS] Compare: %s-%s\n", client->username, target, channel);

        // Check channel name
        if (strcmp(channel, target) == 0) {
            // Prepare auth path
            char path_auth[MAX_BUFFER * 3];
            sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);

            // Open auth file
            FILE *file_auth = fopen(path_auth, "r");

            // Fail if file cannot be opened
            if (file_auth == NULL) {
                // DEBUGGING
                printf("[%s][CHECK PERMS] Error: Unable to open file\n", client->username);

                // Return -1 if file cannot be opened
                return -1;
            }

            // Loop through id, username, and role
            char namecheck[MAX_BUFFER], role[8];
            while (fscanf(file_auth, "%*d,%[^,],%s", namecheck, role) == 2) {
                // DEBUGGING
                printf("[%s][CHECK PERMS] Client username: %s, role: %s\n", client->username, namecheck, role);

                // Check if username matches
                if (strcmp(namecheck, client->username) == 0)
                // Check if role is root/admin
                if (strcmp(role, "ROOT") == 0 || strcmp(role, "ADMIN") == 0) {
                    // DEBUGGING
                    printf("[%s][CHECK PERMS] Error: User is privileged\n", client->username);

                    // Close files
                    fclose(file_auth);
                    fclose(file_channel);

                    // Return 1 if user is privileged
                    return 1;
                }
            }

            // Close auth file
            fclose(file_auth);
        }
    }

    // Close channel file
    fclose(file_channel);

    // Return 0 if user is not privileged
    return 0;
}
```
</details>

### Create Channel
Fungsi `create_channel` bertujuan untuk membuat channel baru dalam sistem. Pertama-tama, fungsi ini memeriksa apakah channel yang diminta sudah ada dengan memanggil fungsi `check_channel`. Jika channel sudah ada, fungsi akan mengirimkan pesan error ke client. Jika tidak, fungsi akan membuka file `channels.csv` dan menambahkan entri baru untuk channel tersebut, termasuk meng-hash key yang diberikan menggunakan `crypt` dengan `HASHCODE`. Setelah itu, fungsi ini membuat direktori baru untuk channel, direktori admin di dalamnya, dan file `auth.csv` di dalam direktori admin. Direktori dan file ini dibuat dengan memanggil fungsi `make_directory` dan `admin_init_channel`. Setelah semua direktori dan file yang diperlukan dibuat, fungsi ini mencatat kejadian pembuatan channel ke dalam log dengan memanggil fungsi `write_log`. Terakhir, fungsi ini mengirimkan pesan sukses ke client yang menunjukkan bahwa channel telah berhasil dibuat, dengan pemberi channel sebagai admin.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// CREATE CHANNEL //
//================//

void create_channel(char *channel, char *key, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if channel id exists
    int id = check_channel(channel, client);

    // DEBUGGING
    printf("[%s][CREATE CHANNEL] id: %d\n", client->username, client->id);

    // Fail if channel already exists
    if (id == -2) {
        // DEBUGGING
        printf("[%s][CREATE CHANNEL] Error: Channel already exists\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Channel %s already exists", channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(channels_csv, "a+");

    // Hash key
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(key, HASHCODE));

    // Write to file
    fprintf(file, "%d,%s,%s\n", id+1, channel, hash);
    fclose(file);

    // Create directory and all related preparations
    char path[MAX_BUFFER * 2], path_admin[MAX_BUFFER * 2], path_auth[MAX_BUFFER * 2];
    sprintf(path, "%s/%s", cwd, channel);
    sprintf(path_admin, "%s/%s/admin", cwd, channel);
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);
    make_directory(path);
    make_directory(path_admin);
    admin_init_channel(path_auth, client);

    // DEBUGGING
    printf("[%s][CREATE CHANNEL] Success: Channel %s created\n", client->username, channel);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s created channel \"%s\"\n", client->username, channel);
    write_log(channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Channel %s created", channel);
    send(client_fd, response, strlen(response), 0);
}
```
</details>

### Edit Channel
Fungsi `edit_channel` bertujuan untuk mengubah nama channel yang sudah ada. Proses ini dimulai dengan memeriksa izin pengguna untuk channel yang akan diubah menggunakan fungsi `check_channel_perms`. Jika izin pengguna tidak mencukupi, fungsi akan mengirim pesan error ke client. Jika izin mencukupi, fungsi akan membuka file `channels.csv` dan file sementara untuk menyimpan data yang telah dimodifikasi. Selanjutnya, fungsi akan membaca setiap entri di `channels.csv` dan membandingkannya dengan nama channel yang akan diubah. Jika ditemukan kecocokan, fungsi akan menulis entri baru dengan nama channel yang baru ke file sementara. Jika tidak ada kecocokan, entri lama akan ditulis ulang ke file sementara. Setelah semua entri diproses, file asli `channels.csv` akan dihapus dan file sementara akan diganti namanya menjadi `channels.csv`. Kemudian, fungsi akan memperbarui nama direktori channel yang diubah dengan memanggil `rename_directory`. Jika channel yang diubah adalah channel yang sedang digunakan oleh client, fungsi juga akan memperbarui nama channel di data client dan mencatat perubahan ini ke dalam log dengan memanggil `write_log`. Terakhir, fungsi akan mengirim pesan sukses ke client yang menunjukkan bahwa nama channel telah berhasil diubah.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//==============//
// EDIT CHANNEL //
//==============//

void edit_channel(char *changed, char *new, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check permissions
    int perms = check_channel_perms(changed, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] User is privileged\n", client->username);
    }

    // Open channel file
    FILE *file = fopen(channels_csv, "r");

    // Open temp channel file
    char temp[MAX_BUFFER * 2];
    sprintf(temp, "%s/.temp_channel.csv", cwd);
    FILE *file_temp = fopen(temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id, channel, and key
    int id; char channel[MAX_BUFFER], key[MAX_BUFFER];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%s", &id, channel, key) == 3) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] id: %d, channel: %s, key: %s\n", client->username, id, channel, key);

        // Write to temp file
        if (strcmp(changed, channel) == 0) {
            found = 1;
            fprintf(file_temp, "%d,%s,%s\n", id, new, key);
        } else {
            fprintf(file_temp, "%d,%s,%s\n", id, channel, key);
        }
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Fail if channel is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Channel not found\n", client->username);

        // Remove temp file
        remove(temp);

        // Send response to client
        sprintf(response, "MSG,Error: Channel not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old channel file and rename temp file
    remove(channels_csv);
    rename(temp, channels_csv);

    // Update folder name
    char path_channel[MAX_BUFFER * 2];
    sprintf(path_channel, "%s/%s", cwd, changed);
    char path_new[MAX_BUFFER * 2];
    sprintf(path_new, "%s/%s", cwd, new);
    rename_directory(path_channel, path_new);

    // Check if edited channel is the current channel
    if (strcmp(client->channel, changed) == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Channel name self-changed\n", client->username);

        // Update client channel
        strcpy(client->channel, new);

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s changed channel \"%s\" name to \"%s\"\n", client->username, changed, new);
        write_log(new, message);

        // Send response to client
        sprintf(response, "CHANNEL,Channel name changed to %s,%s", new, new);
        send(client_fd, response, strlen(response), 0);
        return;
    } 
    
    // DEBUGGING
    printf("[%s][EDIT CHANNEL] Channel name changed\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Channel name changed to %s", new);
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

#### Bonus Case 1
Dalam skenario ini, ketika admin atau root mengubah nama channel yang sedang digunakan, fungsi `edit_channel` secara otomatis memperbarui data client dan mengirim pesan terkait perubahan tersebut. Proses dimulai dengan pemeriksaan izin menggunakan `check_channel_perms`, memastikan pengguna memiliki hak untuk mengedit channel; jika tidak, pesan error dikirim ke client dan fungsi dihentikan. Selanjutnya, fungsi membuka file `channels.csv` dan file sementara untuk menulis data yang dimodifikasi, membaca setiap entri di `channels.csv`, dan membandingkan dengan nama channel yang akan diubah. Jika ditemukan kecocokan, entri baru dengan nama channel yang diubah ditulis ke file sementara; jika tidak, entri lama ditulis ulang. Setelah semua entri diproses, file asli `channels.csv` dihapus dan file sementara diganti namanya menjadi `channels.csv`. Fungsi kemudian mengubah nama direktori channel lama menjadi nama channel baru menggunakan `rename_directory`. Jika nama channel yang diubah adalah nama channel yang sedang digunakan oleh client, data client diperbarui dengan nama channel baru dan pesan log dicatat. Akhirnya, fungsi mengirim pesan ke client yang memberitahukan perubahan nama channel.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
if (strcmp(client->channel, changed) == 0) {
    // DEBUGGING
    printf("[%s][EDIT CHANNEL] Channel name self-changed\n", client->username);

    // Update client channel
    strcpy(client->channel, new);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s changed channel \"%s\" name to \"%s\"\n", client->username, changed, new);
    write_log(new, message);

    // Send response to client
    sprintf(response, "CHANNEL,Channel name changed to %s,%s", new, new);
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

#### Bonus Case 2
Dalam skenario ini, ketika seorang admin atau root mengubah nama channel yang sedang digunakan oleh user lain, fungsi `edit_channel` memastikan bahwa user yang berada di channel lama akan keluar dari channel tersebut secara aman sebelum proses penggantian nama dilanjutkan. Setelah melakukan pemeriksaan izin dan memperbarui file sementara seperti pada Bonus Case 1, fungsi mengubah nama direktori channel lama menjadi nama channel baru dengan menggunakan `rename_directory`. Selanjutnya, fungsi memeriksa apakah ada user lain yang masih berada di dalam channel yang namanya akan diubah. Jika ada, mereka akan dikeluarkan dari channel tersebut dengan mengirim pesan keluar yang mengosongkan nama channel dan room yang mereka gunakan, sehingga memastikan tidak ada konflik command yang terjadi saat proses perubahan nama channel sedang dilakukan.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
// Check if edited channel is the current channel
if (strcmp(client->channel, changed) == 0) {
    // DEBUGGING
    printf("[%s][EDIT CHANNEL] Channel name self-changed\n", client->username);

    // Update client channel
    strcpy(client->channel, new);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s changed channel \"%s\" name to \"%s\"\n", client->username, changed, new);
    write_log(new, message);

    // Send response to client
    sprintf(response, "CHANNEL,Channel name changed to %s,%s", new, new);
    send(client_fd, response, strlen(response), 0);
    return;
} else {
    // DEBUGGING
    printf("[%s][EDIT CHANNEL] Channel name changed for another user\n", client->username);

    // If another user is in the channel, send exit message
    sprintf(response, "EXIT,Channel %s renamed,CHANNEL", changed);
    send(client_fd, response, strlen(response), 0);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s changed channel \"%s\" name to \"%s\"\n", client->username, changed, new);
    write_log(new, message);
}
```
</details>

### Del Channel
Fungsi delete_channel digunakan untuk menghapus channel yang ada. Proses ini dimulai dengan memeriksa izin pengguna untuk channel yang akan dihapus menggunakan fungsi check_channel_perms. Jika izin pengguna tidak mencukupi, fungsi akan mengirim pesan error ke client. Jika izin mencukupi, fungsi akan membuka file channels.csv dan file sementara untuk menyimpan data yang telah dimodifikasi. Selanjutnya, fungsi akan membaca setiap entri di channels.csv dan membandingkannya dengan nama channel yang akan dihapus. Jika tidak ditemukan kecocokan, entri lama akan ditulis ulang ke file sementara. Jika ditemukan kecocokan, entri tersebut tidak akan ditulis ulang, menandakan bahwa channel tersebut telah dihapus. Setelah semua entri diproses, file asli channels.csv akan dihapus dan file sementara akan diganti namanya menjadi channels.csv. Kemudian, fungsi akan menghapus direktori channel yang dihapus dengan memanggil remove_directory. Jika channel yang dihapus adalah channel yang sedang digunakan oleh client, fungsi juga akan memperbarui data client untuk mengosongkan nama channel dan room yang sedang digunakan, serta mengirim pesan keluar ke client. Terakhir, fungsi akan mengirim pesan sukses ke client yang menunjukkan bahwa channel telah berhasil dihapus.
**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
//================//
// DELETE CHANNEL //
//================//

void delete_channel(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check permissions
    int perms = check_channel_perms(channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] User is privileged\n", client->username);
    }

    // Open channel file
    FILE *file = fopen(channels_csv, "r");

    // Open temp channel file
    char temp[MAX_BUFFER * 2];
    sprintf(temp, "%s/.temp_channel.csv", cwd);
    FILE *file_temp = fopen(temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id, channel, and key
    int id; char channelcheck[MAX_BUFFER], key[MAX_BUFFER];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%s", &id, channelcheck, key) == 3) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] id: %d, channel: %s, key: %s\n", client->username, id, channelcheck, key);

        // Write to temp file
        if (strcmp(channel, channelcheck) != 0) {
            fprintf(file_temp, "%d,%s,%s\n", id, channelcheck, key);
        } else {
            found = 1;
        }
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Fail if channel is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Channel not found\n", client->username);

        // Remove temp file
        remove(temp);

        // Send response to client
        sprintf(response, "MSG,Error: Channel not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old channel file and rename temp file
    remove(channels_csv);
    rename(temp, channels_csv);

    // Remove channel directory
    char path_channel[MAX_BUFFER * 2];
    sprintf(path_channel, "%s/%s", cwd, channel);
    remove_directory(path_channel);

    // Check if deleted channel is the current channel
    if (strcmp(client->channel, channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Channel current deleted\n", client->username);

        // Update client channel
        strcpy(client->channel, "");

        // Update client room in case client is in a room
        strcpy(client->room, "");

        // Send response to client
        sprintf(response, "EXIT,Channel %s deleted,CHANNEL", channel);

        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DELETE CHANNEL] Channel deleted\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Channel %s deleted", channel);
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### Bonus Case 1: 

**Admin Menghapus Room yang Sedang Digunakan**

**Penjelasan**: Ketika seorang admin atau root menghapus room yang sedang digunakan oleh klien, maka server harus mengirim pesan kepada klien yang berada di dalam room tersebut untuk memperbarui status room mereka menjadi kosong.

**Alur**:
1. **Admin menghapus room**: Admin mengirim perintah ke server untuk menghapus room.
2. **Server memproses penghapusan room**:
   - Server memeriksa apakah ada klien yang menggunakan room tersebut.
   - Jika ada, server mengirim pesan kepada klien untuk mengosongkan variabel `room`.
3. **Klien menerima pesan dan memperbarui status room**:
   - Klien menerima pesan dari server.
   - Klien mengosongkan variabel `room` dan menampilkan pesan kepada pengguna tentang perubahan status room.

### Bonus Case 2: 

**Mencegah Penghapusan Room "admin"**

**Penjelasan**: Server melarang penghapusan room dengan nama "admin" karena di dalam setiap channel terdapat folder "admin" yang berisi `auth.csv` dan `user.log`.

**Alur**:
1. **Periksa room yang akan dihapus**: Server memeriksa nama room yang akan dihapus.
2. **Tolak permintaan jika room adalah "admin"**: Jika nama room adalah "admin", server menolak permintaan penghapusan.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

   ```c
void delete_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to delete admin
    if (strcmp(room, "admin") == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Invalid room name\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] User is privileged\n", client->username);
    }

    // Prepare room path
    char path_room[MAX_BUFFER * 2];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);

    // Check if room exists
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove room directory
    remove_directory(path_room);

    // Check if deleted room is the current room
    if (strcmp(client->room, room) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Room current deleted %s\n", client->username, room);


        // Update client room
        strcpy(client->room, "");

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "EXIT,Room %s deleted,ROOM %s", room, room);
        
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DELETE ROOM] Room deleted\n", client->username);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Room %s deleted", room);
    send(client_fd, response, strlen(response), 0);
    return;
}
void delete_all_rooms(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is privileged\n", client->username);
    }

    // Prepare channel path
    char path_channel[MAX_BUFFER * 2];
    sprintf(path_channel, "%s/%s", cwd, client->channel);


    // Open channel directory
    DIR *dir = opendir(path_channel);

    // Fail if directory cannot be opened
    if (dir == NULL) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to open directory\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open directory");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through directory
    int rooms_found = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Room: %s\n", client->username, entry->d_name);

        // Skip admin directory
        if (strcmp(entry->d_name, "admin") == 0) continue;

        // Prepare entry path
        char entry_path[MAX_BUFFER * 3];
        sprintf(entry_path, "%s/%s", path_channel, entry->d_name);

        // Check if entry is a directory to be deleted
        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) continue;
        if (S_ISDIR(statbuf.st_mode)) {
            // Increment rooms found
            rooms_found++;

            // Remove room directory
            remove_directory(entry_path);
        }
    }

    // Close directory
    closedir(dir);

    // DEBUGGING
    printf("[%s][DELETE ALL ROOMS] Rooms found: %d\n", client->username, rooms_found);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s deleted all rooms of channel \"%s\"\n", client->username, client->channel);
    write_log(client->channel, message);

    // Check if user is in a room
    if (strlen(client->room) != 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is in a room\n", client->username);

        // Update client room
        strcpy(client->room, "");

        // Send response to client
        sprintf(response, "EXIT,All rooms deleted,ROOM");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        sprintf(response, "MSG,Success: All rooms deleted");
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

```
</details>


### Delete Room All
**Penjelasan**: Fungsi `delete_all_rooms` digunakan untuk menghapus semua room yang ada dalam sebuah channel, kecuali room "admin". Fungsi ini juga memastikan bahwa user memiliki izin yang cukup untuk melakukan tindakan ini dan memperbarui status user jika mereka berada di dalam salah satu room yang dihapus.

**Alur**:
1. **Inisialisasi dan Persiapan Respons**:
   - Inisialisasi variabel `client_fd` untuk menyimpan file descriptor soket klien.
   - Inisialisasi buffer `response` untuk menyiapkan respons yang akan dikirim ke klien.

2. **Memeriksa Jika User Ada di Dalam Channel**:
   - Jika `client->channel` kosong (tidak ada channel yang terhubung), maka:
     - Mencetak pesan kesalahan untuk debugging.
     - Mengirim pesan kesalahan ke klien melalui soket.
     - Mengakhiri fungsi dengan `return`.

3. **Memeriksa Izin User**:
   - Memanggil fungsi `check_channel_perms` untuk memeriksa izin user di channel yang terhubung.
   - Jika izin tidak dapat diperiksa (`perms == -1`), maka:
     - Mencetak pesan kesalahan untuk debugging.
     - Mengirim pesan kesalahan ke klien melalui soket.
     - Mengakhiri fungsi dengan `return`.
   - Jika user tidak memiliki izin yang cukup (`perms == 0`), maka:
     - Mencetak pesan kesalahan untuk debugging.
     - Mengirim pesan kesalahan ke klien melalui soket.
     - Mengakhiri fungsi dengan `return`.
   - Jika user memiliki izin yang cukup (`perms == 1`), maka:
     - Mencetak pesan bahwa user memiliki izin untuk debugging.

4. **Menyiapkan Path Channel**:
   - Menyiapkan string `path_channel` yang merupakan path direktori channel yang terhubung.

5. **Membuka Direktori Channel**:
   - Membuka direktori channel dengan `opendir`.
   - Jika direktori tidak dapat dibuka (`dir == NULL`), maka:
     - Mencetak pesan kesalahan untuk debugging.
     - Mengirim pesan kesalahan ke klien melalui soket.
     - Mengakhiri fungsi dengan `return`.

6. **Loop Melalui Isi Direktori Channel**:
   - Inisialisasi variabel `rooms_found` untuk menghitung jumlah room yang ditemukan.
   - Menggunakan `readdir` untuk membaca isi direktori.
   - Melewati entri `.` dan `..`.
   - Untuk setiap entri:
     - Mencetak nama room untuk debugging.
     - Melewati direktori "admin".
     - Menyiapkan path lengkap untuk setiap entri.
     - Memeriksa apakah entri adalah direktori.
     - Jika entri adalah direktori, maka:
       - Meningkatkan penghitung `rooms_found`.
       - Menghapus direktori room dengan `remove_directory`.

7. **Menutup Direktori**:
   - Menutup direktori dengan `closedir`.

8. **Menulis Log**:
   - Menyiapkan pesan log yang mencatat penghapusan semua room oleh user.
   - Menulis log ke file log channel.

9. **Memeriksa Jika User Ada di Dalam Room**:
   - Jika user berada di dalam room (`client->room` tidak kosong), maka:
     - Mencetak pesan untuk debugging.
     - Mengosongkan variabel `client->room`.
     - Mengirim respons ke klien dengan informasi bahwa semua room dihapus dan user keluar dari room.
     - Mengakhiri fungsi dengan `return`.

10. **Mengirim Respons ke Klien**:
    - Jika tidak ada room yang ditemukan, maka:
      - Mengirim pesan "No rooms found" ke klien.
    - Jika ada room yang dihapus, maka:
      - Mengirim pesan sukses ke klien bahwa semua room telah dihapus.


**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>
   
   ```c

   void delete_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to delete admin
    if (strcmp(room, "admin") == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Invalid room name\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] User is privileged\n", client->username);
    }

    // Prepare room path
    char path_room[MAX_BUFFER * 2];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);

    // Check if room exists
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove room directory
    remove_directory(path_room);

    // Check if deleted room is the current room
    if (strcmp(client->room, room) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Room current deleted %s\n", client->username, room);


        // Update client room
        strcpy(client->room, "");

        // Write to log
        char message[MAX_BUFFER * 2];
        sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "EXIT,Room %s deleted,ROOM %s", room, room);
        
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DELETE ROOM] Room deleted\n", client->username);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Room %s deleted", room);
    send(client_fd, response, strlen(response), 0);
    return;
}

void delete_all_rooms(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is privileged\n", client->username);
    }

    // Prepare channel path
    char path_channel[MAX_BUFFER * 2];
    sprintf(path_channel, "%s/%s", cwd, client->channel);


    // Open channel directory
    DIR *dir = opendir(path_channel);

    // Fail if directory cannot be opened
    if (dir == NULL) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to open directory\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open directory");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through directory
    int rooms_found = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Room: %s\n", client->username, entry->d_name);

        // Skip admin directory
        if (strcmp(entry->d_name, "admin") == 0) continue;

        // Prepare entry path
        char entry_path[MAX_BUFFER * 3];
        sprintf(entry_path, "%s/%s", path_channel, entry->d_name);

        // Check if entry is a directory to be deleted
        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) continue;
        if (S_ISDIR(statbuf.st_mode)) {
            // Increment rooms found
            rooms_found++;

            // Remove room directory
            remove_directory(entry_path);
        }
    }

    // Close directory
    closedir(dir);

    // DEBUGGING
    printf("[%s][DELETE ALL ROOMS] Rooms found: %d\n", client->username, rooms_found);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s deleted all rooms of channel \"%s\"\n", client->username, client->channel);
    write_log(client->channel, message);

    // Check if user is in a room
    if (strlen(client->room) != 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is in a room\n", client->username);

        // Update client room
        strcpy(client->room, "");

        // Send response to client
        sprintf(response, "EXIT,All rooms deleted,ROOM");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        sprintf(response, "MSG,Success: All rooms deleted");
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

```
</details>

 

### Ban
#### Check Ban
**Penjelasan**:
Fungsi `check_ban` bertujuan untuk memeriksa apakah user yang terhubung ke server saat ini telah dibanned dari channel yang mereka coba akses. Fungsi ini mengakses file `auth.csv` di folder `admin` pada channel untuk memeriksa status user.

**Alur**:
1. **Inisialisasi dan Persiapan**:
   - Inisialisasi variabel `client_fd` untuk menyimpan file descriptor soket klien.

2. **Memeriksa Jika User Ada di Dalam Channel**:
   - Fungsi memeriksa apakah panjang `client->channel` adalah 0, yang berarti user tidak terhubung ke channel mana pun.
   - Jika kondisi ini benar, cetak pesan kesalahan untuk debugging dan kembalikan nilai 0 untuk menunjukkan bahwa user tidak berada di channel mana pun.

3. **Membuka File `auth.csv`**:
   - Menyiapkan path lengkap untuk file `auth.csv` di folder `admin` dari channel yang terhubung.
   - Membuka file `auth.csv` dalam mode read-only.
   - Jika file tidak bisa dibuka, cetak pesan kesalahan untuk debugging dan kembalikan nilai -1 untuk menunjukkan bahwa file tidak dapat dibuka.

4. **Looping Melalui File `auth.csv`**:
   - Membaca baris demi baris dari file `auth.csv` menggunakan `fscanf`.
   - Pada setiap baris, membaca ID dan role user dari file.
   - Mencetak ID dan role yang dibaca untuk tujuan debugging.

5. **Memeriksa ID dan Role**:
   - Jika ID yang dibaca dari file sama dengan ID user yang sedang diperiksa:
     - Jika role user adalah "BANNED", cetak pesan kesalahan untuk debugging, tutup file, dan kembalikan nilai 1 untuk menunjukkan bahwa user dibanned.
     - Jika role user bukan "BANNED", break dari loop karena ID sudah cocok tetapi role tidak sesuai.

6. **Menutup File**:
   - Setelah selesai membaca file dan tidak menemukan role "BANNED" untuk user tersebut, tutup file `auth.csv`.

7. **Mencetak Pesan Sukses**:
   - Cetak pesan sukses untuk debugging yang menunjukkan bahwa user tidak dibanned.
   - Kembalikan nilai 0 untuk menunjukkan bahwa user tidak dibanned.

   

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

   ```c
int check_ban(client_data *client) {
    int client_fd = client->socket_fd;

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][CHECK BAN] Error: User is not in a channel\n", client->username);

        // Return 0 if user is not in a channel
        return 0;
    }

    // Open auth file
    char path_auth[MAX_BUFFER * 2];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file = fopen(path_auth, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CHECK BAN] Error: Unable to open file\n", client->username);

        // Return -1 if file cannot be opened
        return -1;
    }

    // Loop through id and username
    int id; char role[8];
    while (fscanf(file, "%d,%*[^,],%s", &id, role) == 2) {
        // DEBUGGING
        printf("[%s][CHECK BAN] id: %d, role: %s\n", client->username, id, role);

        // Check if username matches
        if (id == client->id) 
        // Check if role is banned
        if (strcmp(role, "BANNED") == 0) {
            // DEBUGGING
            printf("[%s][CHECK BAN] Error: User is banned\n", client->username);

            // Close file
            fclose(file);

            // Return 1 if user is banned
            return 1;
        } else break;
    }

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][CHECK BAN] Success: User is not banned\n", client->username);

    // Return 0 if user is not banned
    return 0;

}
```
</details>


## Ban User
#### Penjelasan:
Fungsi `ban_user` digunakan untuk mem-banned user tertentu dari sebuah channel. Fungsi ini mengecek apakah user yang ingin di-banned ada dalam channel, memiliki izin yang cukup, dan bukan seorang admin atau root. Jika user valid ditemukan, maka statusnya diubah menjadi "BANNED".

#### Alur:
1. **Inisialisasi dan Persiapan Respons**:
   - Inisialisasi variabel `client_fd` untuk menyimpan file descriptor soket klien.
   - Inisialisasi buffer `response` untuk menyiapkan respons yang akan dikirim ke klien.
   - Cetak nama user yang ingin di-banned untuk debugging.

2. **Memeriksa Jika User Ada di Dalam Channel**:
   - Jika `client->channel` kosong (tidak ada channel yang terhubung), cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

3. **Memeriksa Izin User**:
   - Memanggil fungsi `check_channel_perms` untuk memeriksa izin user di channel yang terhubung.
   - Jika izin tidak dapat diperiksa, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.
   - Jika user tidak memiliki izin, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.
   - Jika user memiliki izin, cetak pesan sukses untuk debugging.

4. **Memeriksa Jika User Mencoba Membanned Dirinya Sendiri**:
   - Jika `client->username` sama dengan `username` yang ingin di-banned, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

5. **Membuka File `auth.csv` dan File Sementara**:
   - Menyiapkan path lengkap untuk file `auth.csv` di folder `admin` dari channel yang terhubung.
   - Membuka file `auth.csv` dalam mode read-only.
   - Menyiapkan file sementara `.temp_auth.csv` untuk menulis perubahan.
   - Jika file `auth.csv` tidak bisa dibuka, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

6. **Looping Melalui File `auth.csv`**:
   - Membaca baris demi baris dari file `auth.csv` menggunakan `fscanf`.
   - Pada setiap baris, membaca ID, nama user, dan role user dari file.
   - Cetak nama user dan role yang dibaca untuk tujuan debugging.

7. **Memeriksa Nama User dan Role**:
   - Jika nama user yang dibaca dari file sama dengan `username` yang ingin di-banned:
     - Jika role user adalah "ROOT" atau "ADMIN", cetak pesan kesalahan untuk debugging, tutup file, hapus file sementara, kirim pesan kesalahan ke klien, dan kembalikan fungsi.
     - Jika role user bukan "ROOT" atau "ADMIN", tulis perubahan role menjadi "BANNED" ke file sementara dan set flag `found` menjadi 1.
   - Jika nama user tidak cocok, tulis kembali baris yang sama ke file sementara tanpa perubahan.

8. **Menutup File**:
   - Tutup file `auth.csv` dan file sementara setelah selesai membaca file.

9. **Menangani Jika Nama User Tidak Ditemukan**:
   - Jika flag `found` adalah 0 (nama user tidak ditemukan), cetak pesan kesalahan untuk debugging, hapus file sementara, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

10. **Mengganti File `auth.csv` Lama dengan File Sementara**:
    - Hapus file `auth.csv` lama dan ganti dengan file sementara yang baru.

11. **Mencetak Pesan Sukses dan Menulis ke Log**:
    - Cetak pesan sukses untuk debugging yang menunjukkan bahwa user telah di-banned.
    - Tulis pesan ke log bahwa user telah di-banned dari channel.
    - Kirim pesan sukses ke klien.

## Unban User
#### Penjelasan:
Fungsi `unban_user` digunakan untuk menghapus status banned dari user tertentu di sebuah channel. Fungsi ini mengecek apakah user yang ingin di-unbanned ada dalam channel dan memiliki status "BANNED". Jika user valid ditemukan, maka statusnya diubah kembali ke status semula sebelum di-banned.

#### Alur:
1. **Inisialisasi dan Persiapan Respons**:
   - Inisialisasi variabel `client_fd` untuk menyimpan file descriptor soket klien.
   - Inisialisasi buffer `response` untuk menyiapkan respons yang akan dikirim ke klien.
   - Cetak nama user yang ingin di-unbanned untuk debugging.

2. **Memeriksa Jika User Ada di Dalam Channel**:
   - Jika `client->channel` kosong (tidak ada channel yang terhubung), cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

3. **Memeriksa Izin User**:
   - Memanggil fungsi `check_channel_perms` untuk memeriksa izin user di channel yang terhubung.
   - Jika izin tidak dapat diperiksa, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.
   - Jika user tidak memiliki izin, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.
   - Jika user memiliki izin, cetak pesan sukses untuk debugging.

4. **Membuka File `auth.csv` dan File Sementara**:
   - Menyiapkan path lengkap untuk file `auth.csv` di folder `admin` dari channel yang terhubung.
   - Membuka file `auth.csv` dalam mode read-only.
   - Menyiapkan file sementara `.temp_auth.csv` untuk menulis perubahan.
   - Jika file `auth.csv` tidak bisa dibuka, cetak pesan kesalahan untuk debugging, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

5. **Looping Melalui File `auth.csv`**:
   - Membaca baris demi baris dari file `auth.csv` menggunakan `fscanf`.
   - Pada setiap baris, membaca ID, nama user, dan role user dari file.
   - Cetak nama user dan role yang dibaca untuk tujuan debugging.

6. **Memeriksa Nama User dan Role**:
   - Jika nama user yang dibaca dari file sama dengan `username` yang ingin di-unbanned dan role user adalah "BANNED":
     - Tulis perubahan role kembali ke role semula ke file sementara dan set flag `found` menjadi 1.
   - Jika nama user tidak cocok atau role bukan "BANNED", tulis kembali baris yang sama ke file sementara tanpa perubahan.

7. **Menangani Jika Nama User Tidak Ditemukan atau Role Bukan "BANNED"**:
   - Jika flag `found` adalah 0 (nama user tidak ditemukan atau role bukan "BANNED"), cetak pesan kesalahan untuk debugging, hapus file sementara, kirim pesan kesalahan ke klien, dan kembalikan fungsi.

8. **Mengganti File `auth.csv` Lama dengan File Sementara**:
    - Hapus file `auth.csv` lama dan ganti dengan file sementara yang baru.

9. **Mencetak Pesan Sukses dan Menulis ke Log**:
    - Cetak pesan sukses untuk debugging yang menunjukkan bahwa user telah di-unbanned.
    - Tulis pesan ke log bahwa user telah di-unbanned dari channel.
    - Kirim pesan sukses ke klien.



**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

   ```c
void ban_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][BAN USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][BAN USER] User is privileged\n", client->username);
    }

    // Check if user trying to ban self
    if (strcmp(client->username, username) == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is trying to ban self\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is trying to ban self");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open auth of channel
    char path_auth[MAX_BUFFER * 2];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file_auth = fopen(path_auth, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/%s/admin/.temp_auth.csv", cwd, client->channel);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    int id; char namecheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
        // DEBUGGING
        printf("[%s][BAN USER] name: %s, role: %s\n", client->username, namecheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][BAN USER] Success: Username found\n", client->username);

            // Check if role is not root/admin
            if (strcmp(role, "ROOT") == 0 || strcmp(role, "ADMIN") == 0) {
                // DEBUGGING
                printf("[%s][BAN USER] Error: User is root/admin\n", client->username);

                // Close files
                fclose(file_auth);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root/admin");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // Write new role to temp file
            fprintf(temp, "%d,%s,BANNED\n", id, namecheck);
            found = 1;
        } else {
            // Write old role to temp file
            fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
        }
    }

    // Close files
    fclose(file_auth);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(path_auth);
    rename(temp_csv, path_auth);

    // DEBUGGING
    printf("[%s][BAN USER] Success: %s banned\n", client->username, username);

    // Write to log
    char message[MAX_BUFFER * 2];
    sprintf(message, "%s banned user %s from %s\n", client->username, username, client->channel);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: %s banned", username);
    send(client_fd, response, strlen(response), 0);
    return;
}

```
</details>


#### Remove User from Channel
**Penjelasan**: Fungsi `remove_user` digunakan untuk menghapus user tertentu dari channel. Fungsi ini memastikan bahwa user yang ingin dihapus bukanlah user yang sedang menjalankan fungsi, serta user tersebut bukan seorang "ROOT". 

**Alur**:
1. **Validasi User**:
   - Cek apakah user mencoba menghapus dirinya sendiri.
   - Pastikan user yang meminta penghapusan memiliki izin yang cukup (bukan "USER" biasa).

2. **Buka dan Siapkan File**:
   - Buka file `users.csv` untuk membaca data user.
   - Buat file sementara untuk menulis perubahan data user.

3. **Periksa dan Hapus User**:
   - Loop melalui file `users.csv` untuk mencari user yang sesuai.
   - Jika user ditemukan dan bukan "ROOT", abaikan user tersebut dalam penulisan file sementara.
   - Jika user tidak ditemukan, keluarkan pesan kesalahan.

4. **Tutup dan Ganti File**:
   - Tutup file lama dan ganti dengan file sementara.
   - Hapus user dari file otentikasi dengan memanggil fungsi `del_username_auth`.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>
   
```c
void remove_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][REMOVE USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to remove self
    if (strcmp(client->username, username) == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is trying to remove self\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is trying to remove self");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][REMOVE USER] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // Check if role is not root
            if (strcmp(role, "ROOT") == 0) {
                // DEBUGGING
                printf("[%s][REMOVE USER] Error: User is root\n", client->username);

                // Close files
                fclose(file);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("[%s][REMOVE USER] Success: Username found\n", client->username);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][REMOVE USER] Success: User removed\n", client->username);

    // Call del_username_auth
    del_username_auth(username, client);
    return;
}
```
</details>


### User
#### See User
**Penjelasan**: Fungsi `list_user` digunakan untuk menampilkan daftar semua pengguna yang ada dalam sistem. Hanya pengguna dengan peran "ROOT" yang diizinkan untuk menggunakan fungsi ini.

**Alur**:
1. **Validasi Root**:
   - Periksa apakah pengguna yang meminta daftar bukan "ROOT". Jika tidak, kirim pesan kesalahan.

2. **Buka File Pengguna**:
   - Buka file `users.csv` untuk membaca daftar pengguna.

3. **Periksa File**:
   - Jika file tidak dapat dibuka, kirim pesan kesalahan.

4. **Baca dan Daftar Pengguna**:
   - Loop melalui file untuk membaca nama pengguna dan tambahkan nama-nama tersebut ke dalam respon.

5. **Kirim Daftar Pengguna**:
   - Tutup file dan kirim daftar nama pengguna ke client.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void list_user(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER * 2];
    sprintf(response, "MSG,");

    // Check if user is not root
    if (strcmp(client->role, "ROOT") != 0) {
        // DEBUGGING
        printf("[%s][LIST USER] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open users file
    FILE *file = fopen(users_csv, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][LIST USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username
    char namecheck[MAX_BUFFER];
    while (fscanf(file, "%*d,%[^,],%*[^,],%*s)", namecheck) == 1) {
        // DEBUGGING
        printf("[%s][LIST USER] User: %s\n", client->username, namecheck);

        // Concatenate response
        if (strcmp(response, "MSG,") != 0) strcat(response, " ");
        strcat(response, namecheck);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
    return;
}

```
</details>


#### Edit Profile Self (Username)
**Penjelasan**: Fungsi `edit_username` digunakan untuk mengedit nama pengguna yang ada dalam sistem, asalkan pengguna yang melakukan perubahan adalah admin atau root, atau pengguna yang sama yang ingin mengedit namanya sendiri.

**Alur**:
1. **Validasi Peran dan Pengguna**:
   - Periksa apakah pengguna adalah admin atau root, atau apakah pengguna sedang mencoba mengedit nama dirinya sendiri. Jika tidak, kirim pesan kesalahan.

2. **Buka File Pengguna**:
   - Buka file `users.csv` untuk membaca daftar pengguna.

3. **Periksa File**:
   - Jika file tidak dapat dibuka, kirim pesan kesalahan.

4. **Baca dan Edit Nama Pengguna**:
   - Loop melalui file untuk menemukan nama pengguna yang cocok, kemudian tulis nama pengguna baru ke file sementara.

5. **Periksa Nama Pengguna**:
   - Jika nama pengguna tidak ditemukan, kirim pesan kesalahan.

6. **Ganti File**:
   - Hapus file lama dan ganti dengan file sementara yang telah diperbarui.

7. **Panggil `edit_username_auth`**:
   - Panggil fungsi `edit_username_auth` untuk memperbarui nama pengguna dalam file otorisasi yang sesuai.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void edit_username(char *username, char *newusername, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT USERNAME] username: %s, newusername: %s\n", client->username, username, newusername);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME] Success: Username found\n", client->username);

            // Write new username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, newusername, passcheck, role);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT USERNAME] Success: Main username edited\n", client->username);

    // Call edit_username_auth
    edit_username_auth(username, newusername, client);
    return;
}
```
</details>


#### Edit Profile Self (Password)
**Penjelasan**: Fungsi `edit_password` digunakan untuk mengubah kata sandi pengguna yang ada dalam sistem. Pengguna yang melakukan perubahan haruslah admin, root, atau pengguna yang sama yang ingin mengedit kata sandinya sendiri.

**Alur**:
1. **Validasi Peran dan Pengguna**:
   - Periksa apakah pengguna adalah admin atau root, atau apakah pengguna sedang mencoba mengedit kata sandi dirinya sendiri. Jika tidak, kirim pesan kesalahan.

2. **Buka File Pengguna**:
   - Buka file `users.csv` untuk membaca daftar pengguna dan menulis ke file sementara.

3. **Periksa File**:
   - Jika file tidak dapat dibuka, kirim pesan kesalahan.

4. **Baca dan Edit Kata Sandi**:
   - Loop melalui file untuk menemukan nama pengguna yang cocok, kemudian hash kata sandi baru dan tulis ke file sementara.

5. **Periksa Nama Pengguna**:
   - Jika nama pengguna tidak ditemukan, kirim pesan kesalahan.

6. **Ganti File**:
   - Hapus file lama dan ganti dengan file sementara yang telah diperbarui.

7. **Kirim Respons**:
   - Kirim pesan ke pengguna yang menandakan bahwa perubahan kata sandi berhasil.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void edit_password(char *username, char *newpassword, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] username: %s, newpassword: %s\n", client->username, username, newpassword);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r+");

    // Open temp file
    char temp_csv[MAX_BUFFER * 2];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT PASSWORD] Success: Username found\n", client->username);

            // Hash new password
            char hash[MAX_BUFFER];
            strcpy(hash,crypt(newpassword, HASHCODE));

            // Write new password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, hash, role);
            found = 1;
        } else {
            // Write old password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] Success: Password edited\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Password edited");
    send(client_fd, response, strlen(response), 0);
    return;    
}
```
</details>


### Exit
#### Exit Room
**Penjelasan**: User keluar dari room.

**Alur**:
1. **User mengirim perintah keluar dari room**: User mengirim perintah untuk keluar dari room.
2. **Server memproses perintah**: Server memproses permintaan ini dan mengosongkan variabel room di klien.

#### Exit Channel
**Penjelasan**: User keluar dari channel.

**Alur**:
1. **User mengirim perintah keluar dari channel**: User mengirim perintah untuk keluar dari channel.
2. **Server memproses perintah**: Server memproses permintaan ini dan mengosongkan variabel channel di klien.

#### Exit Discorit
**Penjelasan**: User keluar dari aplikasi Discorit.

**Alur**:
1. **User mengirim perintah keluar dari aplikasi**: User mengirim perintah untuk keluar dari aplikasi.
2. **Server memproses perintah**: Server memproses permintaan ini dan memutuskan koneksi klien.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void exit_user(client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][EXIT DISCORIT]\n", client->username);

        // Send response to client
        sprintf(response, "QUIT,Gracefully disconnecting from server...");
        send(client_fd, response, strlen(response), 0);

        // Close client connection
        close(client_fd);
        free(client);
        pthread_exit(NULL);
        return;
    }

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][EXIT CHANNEL] Exiting channel\n", client->username);

        // Exit channel since user is not in a room
        strcpy(client->channel, "");
        sprintf(response, "EXIT,Exited channel %s,CHANNEL", client->channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][EXIT ROOM] Exiting room\n", client->username);

    // Exit room since user is in a room
    strcpy(client->room, "");
    sprintf(response, "EXIT,Exited room %s,ROOM", client->room);
    send(client_fd, response, strlen(response), 0);
    return;
}
```
</details>

### User Log
**Penjelasan**: Fungsi ini digunakan untuk mencatat pesan log ke dalam file `log.csv` yang terletak di direktori admin pada channel tertentu. Ini berarti semua aktivitas yang tercatat di log ini berhubungan dengan manajemen channel tersebut, seperti pembuatan dan penghapusan room, serta aktivitas lain yang relevan dengan pengelolaan channel..

**Alur**:
1. **Persiapan Path Log**: Fungsi mempersiapkan path menuju file log (`log.csv`) di dalam direktori "admin" pada channel yang ditentukan. Path ini dibentuk dengan menggunakan direktori kerja saat ini (`cwd`) dan nama channel yang diberikan sebagai parameter.

2. **Membuka File Log**: File log dibuka dalam mode `a+` (append/update mode), yang memungkinkan penulisan data baru ke akhir file dan memungkinkan juga untuk membaca file jika diperlukan.

3. **Pengecekan Ketersediaan File**: Jika file log tidak dapat dibuka (misalnya, karena masalah akses atau direktori tidak ada), fungsi akan mencetak pesan kesalahan dan menghentikan proses pencatatan log.


**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
void write_log(char* channel, char* message) {
    // Prepare log path
    char path_log[MAX_BUFFER * 2];
    sprintf(path_log, "%s/%s/admin/log.csv", cwd, channel);

    // Open log file
    FILE *file = fopen(path_log, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("[LOG] Error: Unable to open file\n");
        return;
    }

    // Get timestamp
    char *timestamp = get_timestamp();

    // Write to log file
    fprintf(file, "[%s] %s", timestamp, message);

    // Close file
    fclose(file);
    return;
}
```
</details>


### Monitor
**Penjelasan**: Monitor adalah program yang berfungsi untuk memantau aktivitas chat server secara real-time. Program ini memungkinkan pengguna untuk melihat percakapan di saluran dan ruang tertentu serta memberikan kemampuan untuk berpindah antar saluran dan ruang.


### Autentikasi
#### Pemilihan Channel dan Room
**Penjelasan**: Ketika pengguna memberikan perintah, pertama-tama program memeriksa apakah perintah tersebut adalah untuk keluar (EXIT). Jika pengguna berada dalam sebuah ruangan, sistem secara otomatis akan mengosongkan ruangan dengan mengirim perintah EXIT, yang menghapus status pengguna dari ruangan yang sedang aktif.

Selanjutnya, setiap perintah yang mengandung `-channel` diikuti dengan nama channel dan `-room` diikuti dengan nama ruangan akan diproses untuk bergabung ke channel atau ruangan yang sesuai. Ini memastikan bahwa pengguna dapat beralih antara channel dan ruangan dengan lancar sesuai dengan permintaan mereka.

**Alur**:
1. **Pengecekan Command EXIT**: Pertama, fungsi `parse_command` memeriksa apakah perintah yang diberikan adalah EXIT atau bukan. Jika pengguna sedang berada dalam sebuah ruangan (`room` tidak kosong), maka fungsi `parse_command` akan menangani perintah EXIT dengan menjalankan fungsi `handle_command("EXIT")`. Ini mengakibatkan pengosongan `room` dan `channel` yang sesuai dengan keluar dari ruangan.

2. **Penanganan Input -channel dan -room**: Setelah melewati pengecekan EXIT, fungsi `parse_command` mem-parse input pengguna untuk memastikan bahwa command yang diberikan adalah `-channel` diikuti dengan nama channel dan `-room` diikuti dengan nama ruangan. Jika formatnya benar, maka dua request akan disiapkan (`JOIN <channel>` dan `JOIN <room>`) dan diproses menggunakan `handle_command`.

### Monitor Loop
**Penjelasan**: Cara kerja loop monitor yang memperbarui tampilan chat setiap 5 detik jika user berada dalam room.

**Alur**:
1. **Loop monitor**: Loop monitor memperbarui tampilan chat setiap 5 detik.
2. **Thread baru untuk input user**: Jika ada input dari user, thread baru dibuat untuk menangani input tersebut.

### Exiting
**Penjelasan**: Pada bagian exit, terdapat pengecekan menggunakan `strstr` dan `strcmp` untuk menentukan jenis exit yang dilakukan. Hal ini penting karena ada perbedaan antara keluar dari channel dan keluar dari room.

**Alur**:
1. **strstr(buffer, "EXIT")**: Mengecek apakah perintah `EXIT` terdapat dalam buffer. Ini digunakan untuk keluar dari room atau channel saat pengguna berada di dalamnya.
2.  **strcmp(command1, "EXIT")**: Mengecek apakah perintah yang diberikan adalah `EXIT`. Ini digunakan untuk keluar dari program atau mengakhiri sesi monitor.

**Kode**:
<details>
<summary><h3>Klik untuk melihat detail</h3>></summary>

```c
 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

//=========//
// GLOBALS //
//=========//

// Global variables
#define PORT 8080
#define MAX_BUFFER 1024
int server_fd;

// Client info variables
char username[100];
char password[100];
char channel[100]="";
char room[100]="";

// Pre-declaration
void *input_handler(void *arg);
void connect_server();
void clear_terminal();
void parse_command(char *buffer);
int handle_account(const char *buffer);
int handle_command(const char *buffer);

//======//
// MAIN //
//======//

// Main function
int main(int argc, char *argv[]) {
    // Validate arguments:
    if (argc < 5) {
        printf("Usage: ./monitor LOGIN <username> -p <password>"
               "\n(not enough arguments)");
        return 1;
  } if (strcmp(argv[1], "LOGIN") != 0) {
        printf("Usage: ./monitor LOGIN <username> -p <password>"
               "\n(invalid command)");
        return 1;
  } if (strcmp(argv[3], "-p") != 0) {
        printf("Usage: ./monitor LOGIN <username> -p <password>"
                "\n(missing -p flag)");
        return 1;
    }

    // Connect to server
    connect_server();

    // Prepare buffer
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    strcpy(username, argv[2]);
    strcpy(password, argv[4]);
    sprintf(buffer, "LOGIN %s %s", username, password);

    // DEBUGGING
    // printf("Sending: %s\n", buffer);

    // Login check
    if (handle_account(buffer) != 1) {
        close(server_fd);
        return 0;
    }

    // Print user prompt
    while(1){
        // Get chat if client is in a room
        while (strlen(room) > 0) {
            clear_terminal();
            printf("==========================================\n");
            handle_command("SEE CHAT");
            printf("==========================================\n"
                   "[!] Monitor refreshes every 5 seconds\n"
                   "[!] Type EXIT to leave the room\n"
                   "==========================================\n");  

            // Handle user input
            pthread_t tid;
            pthread_create(&tid, NULL, input_handler, NULL);

            // Refresh every 5 seconds
            sleep(5); pthread_cancel(tid);
        }

        // Handle user input
        pthread_t tid;
        pthread_create(&tid, NULL, input_handler, NULL);
        pthread_join(tid, NULL);
    }
}

//=========//
// METHODS //
//=========//

// Input handler function
void *input_handler(void *arg) {
    // Print user prompt
    if (strlen(room) > 0) 
        printf("[%s/%s/%s] ", username, channel, room);
    else if (strlen(channel) > 0) 
        printf("[%s/%s] ", username, channel);
    else 
        printf("[%s] ", username);

    // Get user input
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    // DEBUGGING
    // printf("Sending: %s\n", buffer);

    // Parse command
    parse_command(buffer);
}

// Connect to server
void connect_server() {
    // Connect to server
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    // DEBUGGING
    printf("Connected to server\n");
}

// Clear terminal
void clear_terminal() {
    printf("\033[H\033[J");
}

// Account handler function
int handle_account(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER];
    memset(response, 0, MAX_BUFFER);
    if (recv(server_fd, response, MAX_BUFFER, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Login status sending
    else if (strcmp(type, "LOGIN") == 0){
        printf("%s\n", message);
        return 1;
    }
}

// Parse command
void parse_command(char *buffer) {
    // Parse command
    char *command1 = strtok(buffer, " ");

    // Check if command1 is NULL
    if (command1 == NULL) {
        printf("Error: command is invalid (empty)\n");
        return;
    }

    // Check if user is in a room for EXIT control
    int exit_control = 0;
    if (strlen(room) > 0) {
        if (strstr(buffer, "EXIT") != NULL) {
            exit_control = 1;
        }
    } else {
        if (strcmp(command1, "EXIT") == 0) {
            exit_control = 1;
        }
    }

    // Check command1 not EXIT to skip filtering
    if (exit_control == 0) {
        // Parse command again
        char *cname = strtok(NULL, " ");
        char *command2 = strtok(NULL, " ");
        char *rname = strtok(NULL, " ");

        // DEBUGGING
        // printf("Command1: %s\n", command1);
        // printf("Channel: %s\n", cname);
        // printf("Command2: %s\n", command2);
        // printf("Room: %s\n", rname);
        
        // Check if command is valid
        if (command2 == NULL || cname == NULL || rname == NULL) {
            printf("Error: command is invalid (missing arguments)\n");
            return;
        }

        // Check if command is correct
        if (strcmp(command1, "-channel") != 0
         || strcmp(command2, "-room") != 0) {
            printf("Error: command is invalid (wrong arguments)\n");
            return;
        }

        // Prepare two requests
        char buffer1[MAX_BUFFER];
        char buffer2[MAX_BUFFER];
        memset(buffer1, 0, sizeof(buffer1));
        memset(buffer2, 0, sizeof(buffer2));
        sprintf(buffer1, "JOIN %s", cname);
        sprintf(buffer2, "JOIN %s", rname);

        // Handle two requests
        if (handle_command(buffer1) == 0)
            handle_command(buffer2);
        return;
    }

    // Check if user is in a channel or room
    if (strlen(room) > 0 || strlen(channel) > 0) {
        while (strlen(room) > 0 || strlen(channel) > 0) 
            handle_command("EXIT");
        return; 
    }

    // Handle EXIT command
    if (handle_command(buffer) == 2) {
        close(server_fd);
        exit(EXIT_SUCCESS);
    }
    return;
}

// Command handler function
int handle_command(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER*8];
    memset(response, 0, MAX_BUFFER*8);
    if (recv(server_fd, response, MAX_BUFFER*8, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Channel/room variable sending
    else if (strcmp(type, "CHANNEL") == 0){
        // Parse channel name
        char *channel_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (channel_name == NULL) {
            perror("channel name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy channel name
        strcpy(channel, channel_name);
        printf("%s\n", message);
        return 0;
    } else if (strcmp(type, "ROOM") == 0){
        // Parse room name
        char *room_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (room_name == NULL) {
            perror("room name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy room name
        strcpy(room, room_name);
        printf("%s\n", message);
        return 0;
    }

    // Channel/room exiting
    else if (strcmp(type, "EXIT") == 0){
        // Parse exit type
        char *exit_type = strtok(NULL, ",");

        // Check if parsing is correct
        if (exit_type == NULL) {
            perror("exit type is empty");
            exit(EXIT_FAILURE);
        }

        // Check if exiting channel
        if (strcmp(exit_type, "CHANNEL") == 0) {
            memset(channel, 0, 100);
            // also reset room in case user is in a room
            memset(room, 0, 100);

            // DEBUGGING
            // printf("%s\n", message);
            return 0;
        }

        // Check if exiting room
        else if (strcmp(exit_type, "ROOM") == 0) {
            memset(room, 0, 100);

            // DEBUGGING
            // printf("%s\n", message);
            return 0;
        }
    }

    // When asked for key
    else if (strcmp(type, "KEY") == 0){
        // Send wrong key
        handle_command("X X");
        return 0;
    }

    // Quit message
    else if (strcmp(type, "QUIT") == 0){
        printf("%s\n", message);
        return 2;
    }
}
```
</details>


