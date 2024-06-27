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
ini gambar

### Keterangan Setiap File
ini gambar

# Authentikasi

## Login/Register (Client)

jelasin alur discorit mulai dari connect server, command yang harus dimasukin, cara ngurus buffer, dan pengiriman register/login ke server

## Login/Register (Server)

jelasin alur awal server mulai dari dapetin folder pake cwd, start server, masalah daemon, sampe loop tunggu client supaya dibuat thread buat masing" client yg connect buat ngehandle masing client

jelasin handling buffer yang dikirim client buat register/login dan apa yg terjadi berikutnya

## Fungsi Register di Server

jangan lupa jelasin root itu user id 1 yang pertama daftar, juga jelasin username harus unique dan fitur password encrypt, disimpen di user.csv

## Fungsi Login di Server

jelasin alurnya

## Apa yang terjadi setelah Login

### Client

jelasin apa yg terjadi di handle_account terus jelasin loop di discorit dan secara garis besar apa yg dilakukan handle_command

### Server

jelasin inti apa yg dilakukan handle_input setelah handle_client berhasil login

# Penggunaan DiscorIT

## Listing

### Channel

jelasin kalo alurnya baca dari channels.csv

### Room

jelasin kalo alurnya itu baca semua folder dalam sebuah channel yang namanya selain admin

### User

jelasin kalo alurnya dari auth.csv channel yang sedang dipakai user yang keluarin command

## Joining

### Channel

jelasin alurnya mulai dari ngecek kedudukan user dari auth
* kalo banned skip
* kalo terdaftar lgsg masuk
* kalo ga terdaftar tapi
    * root, langsung masuk
    * else harus minta verifikasi (jelasin teknis cara kerjanya, jelasin juga kalo monitor gaboleh masukin key)

### Room

jelasin alurnya ini cukup simple 

## Chatting

### Get Timestamp

jelasin ini fungsi pendukung banyak fitur

### Send Chat

jelasin alur chat masuk ke csv

### See Chat

jelasin ini baca dari csv

### Edit Chat

jelasin cara kerja edit di temp_file di loop ganti yang sesuai idnya, buat permission root/admin boleh edit semua chat, else cuman boleh edit punya diri sendiri

### Delete Chat

sama kaya edit cuman cara kerjanya pas di temp_file di skip kalo ketemu id yang sama

## Root Actions

### Check User

jelasin alur dan kenapa ini penting

### Editing User (Username)

jelasin sistem edit username, alurnya mirip edit chat, tapi ini ada alur tambahan buat ngeloop lewat auth.csv semua channel buat dicari nama sebelumnya buat diubah

#### Bonus Case 1

Ketika root melakukan edit pada username diri sendiri, maka akan secara otomatis update data client yang tersimpan di server dan juga mengirim pesan ke program client untuk mengubah nama.

#### Bonus Case 2

Ketika seorang user berada dalam sebuah channel, dan seorang root (orang lain) melakukan edit pada nama user, untuk menghindari kerusakan pada sistem, user akan otomatis keluar dari DiscorIT tanpa diban atau keluar dari user terdaftar pada channel. Contoh:

* User join Channel
* Root rename User
* Command berikutnya dari User akan ditolak
* User akan keluar dari DiscorIT secara aman

### Editing User (Password)

alurnya sama tapi gperlu edit di auth.csv

### Remove User Account

alurnya sama kaya edit username cuman ini delete dari temp_file dan semua auth.csv juga.

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

jelasin make directory, rename directory, sama remove directory

### Check Channel Permission

jelasin ini juga fungsi pendukung

### Create Channel

alurnya masukin ke csv, ngehash key, bikin folder baru, terus bikin auth.log yang isinya pembuat channel sbg admin

### Edit Channel

seperti biasa, pake teknik edit biasanya, cuman di akhir ada juga rename directory

#### Bonus Case 1

Ketika seorang admin/root melakukan edit pada nama channel yang sedang digunakan pada saat itu, maka secara otomatis akan melakukan update pada client data dan mengirim pesan pada client atas perubahannya.

#### Bonus Case 2

Ketika seorang admin/root melakukan edit pada nama channel yang sedang digunakan oleh user lain, untuk menghindari konflik command, maka user yang sedang berada di dalam nama channel lama akan keluar dari channel secara aman sebelum command berikutnya terkirim.

### Del Channel

seperti biasa, tapi di akhir ada remove directory

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
**Penjelasan**: Menghapus semua room kecuali room "admin".

**Alur**:
1. **Loop melalui semua room**: Server melakukan loop melalui semua room yang ada.
2. **Hapus room kecuali "admin"**: Jika room bukan "admin", server menghapus room tersebut.

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
**Penjelasan**: Memeriksa apakah user diban atau tidak. Proses ini juga digunakan untuk memeriksa apakah channel masih ada atau tidak.

**Alur**:
1. **Pemeriksaan status ban**: Server memeriksa status ban user di database atau file yang relevan.
2. **Kirim pesan status**: Server mengirim pesan ke user atau admin terkait status ban.

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


#### Ban User
**Penjelasan**: Mengubah status user menjadi banned.

**Alur**:
1. **Server menerima permintaan ban**: Admin mengirim permintaan untuk memban user.
2. **Perbarui status user**: Server memperbarui status user menjadi banned di database atau file yang relevan.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang diban.

#### Unban User
**Penjelasan**: Mengubah status user menjadi unbanned.

**Alur**:
1. **Server menerima permintaan unban**: Admin mengirim permintaan untuk meng-unban user.
2. **Perbarui status user**: Server memperbarui status user menjadi unbanned di database atau file yang relevan.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang di-unban.

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
**Penjelasan**: Menghapus user dari channel dengan menghapus entri dari file `auth.csv`.

**Alur**:
1. **Server menerima permintaan penghapusan user**: Admin mengirim permintaan untuk menghapus user dari channel.
2. **Hapus entri user dari `auth.csv`**: Server menghapus entri user dari file `auth.csv` yang berisi daftar user yang memiliki akses ke channel tersebut.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang dihapus.

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
**Penjelasan**: Melihat data user dan variabel yang relevan saat testing.

**Alur**:
1. **User mengirim permintaan**: User mengirim permintaan untuk melihat User.
2. **Server mengirim data pribadi**: Server mengirim data user.

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
**Penjelasan**: Mengubah username sendiri.

**Alur**:
1. **User mengirim permintaan perubahan username**: User mengirim permintaan untuk mengubah username.
2. **Server memperbarui username**: Server memperbarui username di database atau file yang relevan.

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
**Penjelasan**: Mengubah password sendiri.

**Alur**:
1. **User mengirim permintaan perubahan password**: User mengirim permintaan untuk mengubah password.
2. **Server memperbarui password**: Server memperbarui password di database atau file yang relevan.

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


