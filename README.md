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

#### Bonus Case 1

Ketika seorang admin/root melakukan delete pada channel yang sedang digunakan pada saat itu, maka secara otomatis akan melakukan update pengosongan nama channel (dan room) pada client data dan mengirim pesan pada client untuk perubahan channel.

#### Bonus Case 2

Ketika seorang admin/root melakukan delete pada channel yang sedang digunakan oleh user lain, untuk menghindari konflik command, maka user yang sedang berada di dalam nama channel lama akan keluar dari channel secara aman sebelum command berikutnya terkirim.

### Create Room

jelasin alurnya bikin folder baru

#### Bonus Case

Karena dalam setiap channel terdapat sebuah folder "admin" yang berisi auth.csv dan user.log, maka server melarang pembuatan Room atas nama "admin".

### Edit Room

intinya rename directory

#### Bonus Case 1

Ketika seorang admin/root melakukan edit pada nama room yang sedang digunakan pada saat itu, maka secara otomatis akan melakukan update pada client data dan mengirim pesan pada client atas perubahannya.

#### Bonus Case 2

Karena dalam setiap channel terdapat sebuah folder "admin" yang berisi auth.csv dan user.log, maka server melarang edit Room dengan atau menjadi nama "admin".

### Delete Room

intinya remove directory dan gaboleh admin

#### Bonus Case 1

Ketika seorang admin/root melakukan delete pada room yang sedang digunakan pada saat itu, maka secara otomatis akan melakukan update pengosongan nama room pada client data dan mengirim pesan pada client untuk perubahan room.

#### Bonus Case 2

Karena dalam setiap channel terdapat sebuah folder "admin" yang berisi auth.csv dan user.log, maka server melarang delete Room atas nama "admin".

### Delete Room All

intinya sama cuman di loop sampe semua folder selain admin hilang

## Ban

### Check Ban

jelasin alur ngecek seseorang diban atau gk, ini juga secara gk langsung dipake buat ngecek kalo sebuah channel itu masi ada atau gk

### Ban User

jelasin alur ngubah ban

### Unban User

sama kaya ban tapi ubah jd user

### Remove User from Channel

sama kaya ban tapi di delete dari auth.csv

## User

### See User

jelasin ini dipake buat ngecek data pribadi sekaligus ngecek variabel pada saat testing

### Edit Profile Self (Username)

literally persis edit user tapi targetnya diri sendiri secara default

### Edit Profile Self (Password)

sama

## Exit

jelasin kalo exit ada 3 tahap

### Exit Room

jelasin

### Exit Channel

jelasin

### Exit Discorit

jelasin

## User Log

jelasin di userlog apa aja yg dicatet dan kalo yg dicatet itu terhubung sama channel userlog itu berada

# Monitor

jelasin apa itu monitor

## Autentikasi

## Pemilihan Channel dan Room

jelasin ttg handling command itu dicek dulu antara 
EXIT atau gk, terus sebut apa yg terjadi setelah input -channel dan -room

## Monitor Loop

jelasin cara kerja loop monitor

## Exiting

jelasin kenapa ada strstr dan strcmp buat perbedaan jenis exit (karena refresh di monitor loop bikin tulisan hilang)



