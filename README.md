# Sisop-FP-2024-MH-IT29

## Anggota Kelompok:
- Dian Anggraeni Putri (5027231016)
- Amoes Noland (5027231028)
- Malvin Putra Rismahardian (5027231048)

# WORK IN PROGRESS !!!

# Pendahuluan

(jelasin singkat tentang persoalan) buat bagian" berikutnya jgn lupa dokumentasi screenshot ataupun bikin gif kalo niat

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

**Kode**:
```c
// Pseudocode for handling room deletion in server
void handle_delete_room(const char *room_name) {
    if (strcmp(room_name, "admin") == 0) {
        printf("Cannot delete 'admin' room.\n");
        return;
    }

    // Find all clients in the room
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL && strcmp(clients[i]->room, room_name) == 0) {
            // Send message to client to leave the room
            send(clients[i]->sockfd, "ROOM DELETED,Your room has been deleted by admin", strlen("ROOM DELETED,Your room has been deleted by admin"), 0);
            // Clear room name on client
            memset(clients[i]->room, 0, sizeof(clients[i]->room));
        }
    }

    // Proceed with room deletion on server
    // Code to delete the room from server records
}
```

### Bonus Case 2: 

**Mencegah Penghapusan Room "admin"**

**Penjelasan**: Server melarang penghapusan room dengan nama "admin" karena di dalam setiap channel terdapat folder "admin" yang berisi `auth.csv` dan `user.log`.

**Alur**:
1. **Periksa room yang akan dihapus**: Server memeriksa nama room yang akan dihapus.
2. **Tolak permintaan jika room adalah "admin"**: Jika nama room adalah "admin", server menolak permintaan penghapusan.

**Kode**:
```c
// Part of the delete room function in server
void handle_delete_room(const char *room_name) {
    if (strcmp(room_name, "admin") == 0) {
        printf("Cannot delete 'admin' room.\n");
        return;
    }

    // Proceed with room deletion on server
    // Code to delete the room from server records
}
```

### Delete Room All
**Penjelasan**: Menghapus semua room kecuali room "admin".

**Alur**:
1. **Loop melalui semua room**: Server melakukan loop melalui semua room yang ada.
2. **Hapus room kecuali "admin"**: Jika room bukan "admin", server menghapus room tersebut.

**Kode**:
```c
// Pseudocode for deleting all rooms except "admin"
void handle_delete_all_rooms() {
    for (int i = 0; i < total_rooms; ++i) {
        if (strcmp(rooms[i], "admin") != 0) {
            handle_delete_room(rooms[i]);
        }
    }
}
```

### Ban
#### Check Ban
**Penjelasan**: Memeriksa apakah user diban atau tidak. Proses ini juga digunakan untuk memeriksa apakah channel masih ada atau tidak.

**Alur**:
1. **Pemeriksaan status ban**: Server memeriksa status ban user di database atau file yang relevan.
2. **Kirim pesan status**: Server mengirim pesan ke user atau admin terkait status ban.

**Kode**:
```c
// Pseudocode for checking if a user is banned
int is_user_banned(const char *username) {
    // Code to check if user is banned in database or file
    // Return 1 if banned, 0 otherwise
}

// Example usage in server function
void handle_check_ban(const char *username) {
    if (is_user_banned(username)) {
        printf("%s is banned.\n", username);
    } else {
        printf("%s is not banned.\n", username);
    }
}
```

#### Ban User
**Penjelasan**: Mengubah status user menjadi banned.

**Alur**:
1. **Server menerima permintaan ban**: Admin mengirim permintaan untuk memban user.
2. **Perbarui status user**: Server memperbarui status user menjadi banned di database atau file yang relevan.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang diban.

**Kode**:
```c
// Pseudocode for banning a user
void ban_user(const char *username) {
    // Code to update user status to banned in database or file
    printf("%s has been banned.\n", username);
}

// Example usage in server function
void handle_ban_user(const char *username) {
    ban_user(username);
    send_user_message(username, "You have been banned.");
    send_admin_message("User has been banned.");
}
```

#### Unban User
**Penjelasan**: Mengubah status user menjadi unbanned.

**Alur**:
1. **Server menerima permintaan unban**: Admin mengirim permintaan untuk meng-unban user.
2. **Perbarui status user**: Server memperbarui status user menjadi unbanned di database atau file yang relevan.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang di-unban.

**Kode**:
```c
// Pseudocode for unbanning a user
void unban_user(const char *username) {
    // Code to update user status to unbanned in database or file
    printf("%s has been unbanned.\n", username);
}

// Example usage in server function
void handle_unban_user(const char *username) {
    unban_user(username);
    send_user_message(username, "You have been unbanned.");
    send_admin_message("User has been unbanned.");
}
```

#### Remove User from Channel
**Penjelasan**: Menghapus user dari channel dengan menghapus entri dari file `auth.csv`.

**Alur**:
1. **Server menerima permintaan penghapusan user**: Admin mengirim permintaan untuk menghapus user dari channel.
2. **Hapus entri user dari `auth.csv`**: Server menghapus entri user dari file `auth.csv` yang berisi daftar user yang memiliki akses ke channel tersebut.
3. **Kirim pesan ke user dan admin**: Server mengirim pesan konfirmasi ke admin dan pesan pemberitahuan ke user yang dihapus.

**Kode**:
```c
// Pseudocode for removing a user from a channel
void remove_user_from_channel(const char *channel, const char *username) {
    // Code to remove user entry from auth.csv file
    printf("%s has been removed from %s.\n", username, channel);
}

// Example usage in server function
void handle_remove_user(const char *channel, const char *username) {
    remove_user_from_channel(channel, username);
    send_user_message(username, "You have been removed from the channel.");
    send_admin_message("User has been removed from the channel.");
}
```

### User
#### See User
**Penjelasan**: Melihat data pribadi user dan variabel yang relevan saat testing.

**Alur**:
1. **User mengirim permintaan**: User mengirim permintaan untuk melihat data pribadi.
2. **Server mengirim data pribadi**: Server mengirim data pribadi user yang bersangkutan.

**Kode**:
```c
// Pseudocode for seeing user data
void see_user_data(const char *username) {
    // Code to retrieve and send user data to the client
    printf("User data for %s sent.\n", username);
}

// Example usage in server function
void handle_see_user_data(const char *username) {
    see_user_data(username);
}
```

#### Edit Profile Self (Username)
**Penjelasan**: Mengubah username sendiri.

**Alur**:
1. **User mengirim permintaan perubahan username**: User mengirim permintaan untuk mengubah username.
2. **Server memperbarui username**: Server memperbarui username di database atau file yang relevan.

**Kode**:
```c
// Pseudocode for editing own username
void edit_own_username(const char *old_username, const char *new_username) {
    // Code to update username in database or file
    printf("%s has changed their username to %s.\n", old_username, new_username);
}

// Example usage in server function
void handle_edit_own_username(const char *old_username, const char *new_username) {
    edit_own_username(old_username, new_username);
}
```

#### Edit Profile Self (Password)
**Penjelasan**: Mengubah password sendiri.

**Alur**:
1. **User mengirim permintaan perubahan password**: User mengirim permintaan untuk mengubah password.
2. **Server memperbarui password**: Server memperbarui password di database atau file yang relevan.

**Kode

**:
```c
// Pseudocode for editing own password
void edit_own_password(const char *username, const char *new_password) {
    // Code to update password in database or file
    printf("%s has changed their password.\n", username);
}

// Example usage in server function
void handle_edit_own_password(const char *username, const char *new_password) {
    edit_own_password(username, new_password);
}
```

### Exit
#### Exit Room
**Penjelasan**: User keluar dari room.

**Alur**:
1. **User mengirim perintah keluar dari room**: User mengirim perintah untuk keluar dari room.
2. **Server memproses perintah**: Server memproses permintaan ini dan mengosongkan variabel room di klien.

**Kode**:
```c
// Pseudocode for exiting a room
void exit_room(const char *username) {
    // Code to clear room variable for the user
    printf("%s has exited the room.\n", username);
}

// Example usage in server function
void handle_exit_room(const char *username) {
    exit_room(username);
}
```

#### Exit Channel
**Penjelasan**: User keluar dari channel.

**Alur**:
1. **User mengirim perintah keluar dari channel**: User mengirim perintah untuk keluar dari channel.
2. **Server memproses perintah**: Server memproses permintaan ini dan mengosongkan variabel channel di klien.

**Kode**:
```c
// Pseudocode for exiting a channel
void exit_channel(const char *username) {
    // Code to clear channel variable for the user
    printf("%s has exited the channel.\n", username);
}

// Example usage in server function
void handle_exit_channel(const char *username) {
    exit_channel(username);
}
```

#### Exit Discorit
**Penjelasan**: User keluar dari aplikasi Discorit.

**Alur**:
1. **User mengirim perintah keluar dari aplikasi**: User mengirim perintah untuk keluar dari aplikasi.
2. **Server memproses perintah**: Server memproses permintaan ini dan memutuskan koneksi klien.

**Kode**:
```c
// Pseudocode for exiting the application
void exit_discorit(const char *username) {
    // Code to handle user exit from the application
    printf("%s has exited Discorit.\n", username);
}

// Example usage in server function
void handle_exit_discorit(const char *username) {
    exit_discorit(username);
}
```

### User Log
**Penjelasan**: Mencatat berbagai aktivitas user seperti login, logout, penghapusan room, dll., di `user.log`.

**Alur**:
1. **Aktivitas user dicatat**: Server mencatat aktivitas user di file `user.log`.
2. **Catatan terhubung dengan channel**: Catatan ini bisa dihubungkan dengan channel yang relevan.

**Kode**:
```c
// Pseudocode for logging user activity
void log_user_activity(const char *username, const char *activity) {
    // Code to write user activity to user.log
    printf("Logged activity for %s: %s\n", username, activity);
}

// Example usage in server function
void handle_user_activity(const char *username, const char *activity) {
    log_user_activity(username, activity);
}
```

### Monitor
**Penjelasan**: Monitor adalah fitur yang memperbarui tampilan chat setiap 5 detik jika user berada dalam room.

**Alur**:
1. **Monitor loop**: Server memperbarui tampilan chat setiap 5 detik.
2. **Input dari user**: Jika ada input dari user, thread baru dibuat untuk menangani input tersebut.

**Kode**:
```c
// Pseudocode for monitor loop
void monitor_loop() {
    while (1) {
        // Code to update chat display every 5 seconds
        sleep(5);
        // Code to handle user input in a new thread
    }
}

// Example usage in server function
void handle_monitor() {
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_loop, NULL);
}
```

### Autentikasi
#### Pemilihan Channel dan Room
**Penjelasan**: Menangani perintah dengan memeriksa apakah perintah tersebut adalah "EXIT" atau tidak, kemudian memproses input channel dan room.

**Alur**:
1. **Periksa perintah EXIT**: Server memeriksa apakah perintah yang diberikan oleh user adalah "EXIT".
2. **Proses input channel dan room**: Jika bukan perintah "EXIT", server memproses input untuk memilih channel dan room.

**Kode**:
```c
// Pseudocode for handling command
void handle_command(const char *command) {
    if (strcmp(command, "EXIT") == 0) {
        // Code to handle exit command
        return;
    }

    // Code to handle channel and room selection
    if (strstr(command, "-channel") != NULL) {
        // Code to handle channel selection
    }

    if (strstr(command, "-room") != NULL) {
        // Code to handle room selection
    }
}
```

### Monitor Loop
**Penjelasan**: Cara kerja loop monitor yang memperbarui tampilan chat setiap 5 detik jika user berada dalam room.

**Alur**:
1. **Loop monitor**: Loop monitor memperbarui tampilan chat setiap 5 detik.
2. **Thread baru untuk input user**: Jika ada input dari user, thread baru dibuat untuk menangani input tersebut.

**Kode**:
```c
// Pseudocode for monitor loop
void monitor_loop() {
    while (1) {
        // Code to update chat display every 5 seconds
        sleep(5);
        // Code to handle user input in a new thread
    }
}

// Example usage in server function
void handle_monitor() {
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_loop, NULL);
}
```

### Exiting
**Penjelasan**: Menggunakan `strstr` dan `strcmp` untuk membedakan jenis exit karena tampilan di monitor loop diperbarui setiap 5 detik, sehingga jenis exit perlu ditangani secara tepat untuk memastikan keluarnya user tercatat dengan benar.

**Alur**:
1. **Periksa jenis exit**: Menggunakan `strstr` dan `strcmp` untuk memeriksa jenis exit.
2. **Proses exit sesuai jenisnya**: Server memproses exit sesuai dengan jenis exit yang terdeteksi.

**Kode**:
```c
// Pseudocode for exiting
void handle_exit(const char *command) {
    if (strcmp(command, "EXIT ROOM") == 0) {
        // Code to handle exit room
    } else if (strcmp(command, "EXIT CHANNEL") == 0) {
        // Code to handle exit channel
    } else if (strcmp(command, "EXIT DISCORIT") == 0) {
        // Code to handle exit Discorit
    }
}

// Example usage in server function
void handle_command(const char *command) {
    if (strstr(command, "EXIT") != NULL) {
        handle_exit(command);
        return;
    }

    // Other command handling code
}
```



