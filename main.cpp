#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/datetime.h>
#include <cups/cups.h>
#include <wx/file.h>
#include <sqlite3.h>  // Include SQLite header

class KasirApp : public wxApp {
public:
    virtual bool OnInit();
};

class KasirFrame : public wxFrame {
public:
    KasirFrame(const wxString& title);
    virtual ~KasirFrame(); 

private:
    wxTextCtrl* txtSearchID;
    wxTextCtrl* txtJumlah;
    wxTextCtrl* txtBayar;
    wxGrid* grid;
    wxStaticText* lblTotal;

    sqlite3* db;  // Database connection
    const char* dbName = "kasir.db"; // SQLite database name

    void OnTambahKeKeranjang(wxCommandEvent& event);
    void ResetTransaksi();
    void OnProsesTransaksi(wxCommandEvent& event);
    void OnHapusTabel(wxCommandEvent& event);
    void SimpanTransaksiKeLog(double total, double bayar, double kembalian);
    void SimpanStrukKeFile(const wxString& struk);
    void CetakStruk();
    double HitungTotal();
    bool AmbilDataBarang(const wxString& idBarang, wxString& namaBarang, double& harga); // Declare the function
    void SetupDatabase();  // Function to setup the database
};

IMPLEMENT_APP(KasirApp)

bool KasirApp::OnInit() {
    KasirFrame* frame = new KasirFrame("Aplikasi Kasir - Full Screen");
    frame->ShowFullScreen(true);
    frame->Show(true);
    return true;
}

KasirFrame::KasirFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1280, 720)) {

    // Open database
    sqlite3_open(dbName, &db);

    // Create the table if it doesn't exist
    SetupDatabase();

    // Menu Bar
    wxMenuBar* menuBar = new wxMenuBar;
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "&Transaksi Baru");
    menuFile->Append(wxID_EXIT, "&Keluar");
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);

    // Main Sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header Panel: Nama Toko dan Alamat
    wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* lblNamaToko = new wxStaticText(headerPanel, wxID_ANY, "FJ FOTOCOPY", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    lblNamaToko->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    headerSizer->Add(lblNamaToko, 0, wxALIGN_CENTER | wxALL, 5);

    wxStaticText* lblAlamatToko = new wxStaticText(headerPanel, wxID_ANY, "KP.Pasirwangi RT01/RW11 Desa Gudang Kahuripan Kecamatan Lembang Kabupaten Bandung Barat", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    lblAlamatToko->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    headerSizer->Add(lblAlamatToko, 0, wxALIGN_CENTER | wxALL, 5);

    headerPanel->SetSizer(headerSizer);

    // Content Sizer
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);

    // Kiri: Input ID Barang, Jumlah
    wxPanel* inputPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* inputSizer = new wxBoxSizer(wxVERTICAL);

    inputSizer->Add(new wxStaticText(inputPanel, wxID_ANY, "ID Barang:"), 0, wxALL, 5);
    txtSearchID = new wxTextCtrl(inputPanel, wxID_ANY);
    inputSizer->Add(txtSearchID, 0, wxEXPAND | wxALL, 5);

    inputSizer->Add(new wxStaticText(inputPanel, wxID_ANY, "Jumlah Barang:"), 0, wxALL, 5);
    txtJumlah = new wxTextCtrl(inputPanel, wxID_ANY);
    inputSizer->Add(txtJumlah, 0, wxEXPAND | wxALL, 5);

    wxButton* btnTambah = new wxButton(inputPanel, wxID_ANY, "Tambah ke Keranjang");
    inputSizer->Add(btnTambah, 0, wxEXPAND | wxALL, 5);

    wxButton* btnHapusTabel = new wxButton(inputPanel, wxID_ANY, "Hapus Tabel");
    inputSizer->Add(btnHapusTabel, 0, wxEXPAND | wxALL, 5);

    inputPanel->SetSizer(inputSizer);

    // Event Handler untuk Tombol Tambah dan Hapus
    btnTambah->Bind(wxEVT_BUTTON, &KasirFrame::OnTambahKeKeranjang, this);
    btnHapusTabel->Bind(wxEVT_BUTTON, &KasirFrame::OnHapusTabel, this);

    // Tengah: Daftar Barang
    wxPanel* tablePanel = new wxPanel(this, wxID_ANY);
    grid = new wxGrid(tablePanel, wxID_ANY);
    grid->CreateGrid(0, 4);
    grid->SetColLabelValue(0, "ID Barang");
    grid->SetColLabelValue(1, "Nama Barang");
    grid->SetColLabelValue(2, "Harga");
    grid->SetColLabelValue(3, "Jumlah");
    wxBoxSizer* tableSizer = new wxBoxSizer(wxVERTICAL);
    tableSizer->Add(grid, 1, wxEXPAND | wxALL, 5);
    tablePanel->SetSizer(tableSizer);

    // Kanan: Ringkasan Transaksi dan Input Bayar + Proses
    wxPanel* summaryPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* summarySizer = new wxBoxSizer(wxVERTICAL);
    
    lblTotal = new wxStaticText(summaryPanel, wxID_ANY, "Total Belanja: Rp. 0");
    summarySizer->Add(lblTotal, 0, wxALL, 5);
    
    summarySizer->Add(new wxStaticText(summaryPanel, wxID_ANY, "Jumlah Bayar:"), 0, wxALL, 5);
    txtBayar = new wxTextCtrl(summaryPanel, wxID_ANY);
    summarySizer->Add(txtBayar, 0, wxEXPAND | wxALL, 5);

    wxButton* btnProsesTransaksi = new wxButton(summaryPanel, wxID_ANY, "Proses Transaksi");
    summarySizer->Add(btnProsesTransaksi, 0, wxEXPAND | wxALL, 5);

    summaryPanel->SetSizer(summarySizer);

    // Event Handler untuk Tombol Proses Transaksi
    btnProsesTransaksi->Bind(wxEVT_BUTTON, &KasirFrame::OnProsesTransaksi, this);

    // Tambahkan Panel Konten ke Content Sizer
    contentSizer->Add(inputPanel, 1, wxEXPAND | wxALL, 5);
    contentSizer->Add(tablePanel, 3, wxEXPAND | wxALL, 5);
    contentSizer->Add(summaryPanel, 1, wxEXPAND | wxALL, 5);

    // Tambahkan Header dan Konten ke Main Sizer
    mainSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 5);

    // Set Main Sizer
    this->SetSizer(mainSizer);

    // Status Bar
    CreateStatusBar();
    SetStatusText("Selamat datang di Aplikasi Kasir!");
}


void KasirFrame::SetupDatabase() {
    const char* createBarangTableSQL = 
        "CREATE TABLE IF NOT EXISTS barang ("
        "id TEXT PRIMARY KEY, "
        "nama TEXT NOT NULL, "
        "harga REAL NOT NULL, "
        "jumlah INTEGER NOT NULL);";

    const char* createTransaksiLogTableSQL = 
        "CREATE TABLE IF NOT EXISTS transaksi_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_barang TEXT NOT NULL, "
        "nama_barang TEXT NOT NULL, "
        "jumlah INTEGER NOT NULL, "
        "total REAL NOT NULL, "
        "bayar REAL NOT NULL, "
        "kembalian REAL NOT NULL, "
        "tanggal TEXT NOT NULL);";

    sqlite3_exec(db, createBarangTableSQL, 0, 0, 0);
    sqlite3_exec(db, createTransaksiLogTableSQL, 0, 0, 0);
}


double KasirFrame::HitungTotal() {
    double total = 0;
    int rowCount = grid->GetNumberRows();
    for (int i = 0; i < rowCount; ++i) {
        wxString hargaStr = grid->GetCellValue(i, 2);
        wxString jumlahStr = grid->GetCellValue(i, 3);
        double harga = wxAtof(hargaStr);
        long jumlah;
        jumlahStr.ToLong(&jumlah);
        total += harga * jumlah;
    }
    return total;
}
bool KasirFrame::AmbilDataBarang(const wxString& idBarang, wxString& namaBarang, double& harga) {
    if (idBarang.Length() > 20) { // Misalnya, batasi panjang ID barang hingga 20 karakter
        wxMessageBox("ID Barang terlalu panjang.", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    sqlite3_stmt* stmt;
    const char* querySQL = "SELECT nama, harga FROM barang WHERE id = ?;";
    
    int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        wxMessageBox("Gagal menyiapkan query.", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    sqlite3_bind_text(stmt, 1, idBarang.ToStdString().c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        namaBarang = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        harga = sqlite3_column_double(stmt, 1);
        sqlite3_finalize(stmt);
        return true;
    } else {
        wxMessageBox("Barang tidak ditemukan.", "Error", wxOK | wxICON_ERROR);
        sqlite3_finalize(stmt);
        return false;
    }
}



void KasirFrame::OnTambahKeKeranjang(wxCommandEvent& event) {
    wxString idBarang = txtSearchID->GetValue();
    wxString jumlahStr = txtJumlah->GetValue();

    if (idBarang.IsEmpty() || jumlahStr.IsEmpty()) {
        wxMessageBox("ID Barang dan Jumlah Barang harus diisi.", "Peringatan", wxOK | wxICON_WARNING);
        return;
    }

    long jumlah;
    if (!jumlahStr.ToLong(&jumlah) || jumlah <= 0) {
        wxMessageBox("Jumlah Barang harus berupa angka positif.", "Peringatan", wxOK | wxICON_WARNING);
        return;
    }

    wxString namaBarang;
    double harga;

    // Fetch product data from database
    if (!AmbilDataBarang(idBarang, namaBarang, harga)) {
        return; // Exit if the product was not found
    }

    // Check current stock
    sqlite3_stmt* stmt;
    const char* checkStockSQL = "SELECT jumlah FROM barang WHERE id = ?;";
    int rc = sqlite3_prepare_v2(db, checkStockSQL, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        wxMessageBox("Gagal memeriksa stok barang.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    sqlite3_bind_text(stmt, 1, idBarang.ToStdString().c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        wxMessageBox("Barang tidak ditemukan di database.", "Error", wxOK | wxICON_ERROR);
        sqlite3_finalize(stmt);
        return;
    }

    int currentStock = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (jumlah > currentStock) {
        wxMessageBox("Stok barang tidak mencukupi.", "Peringatan", wxOK | wxICON_WARNING);
        return;
    }

    // Reduce stock in database
    const char* updateStockSQL = "UPDATE barang SET jumlah = jumlah - ? WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, updateStockSQL, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        wxMessageBox("Gagal memperbarui stok barang.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    sqlite3_bind_int(stmt, 1, jumlah);
    sqlite3_bind_text(stmt, 2, idBarang.ToStdString().c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        wxMessageBox("Gagal mengurangi stok barang.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Add to grid
    int row = grid->GetNumberRows();
    grid->AppendRows(1);
    grid->SetCellValue(row, 0, idBarang);
    grid->SetCellValue(row, 1, namaBarang);
    grid->SetCellValue(row, 2, wxString::Format("%.2f", harga));
    grid->SetCellValue(row, 3, wxString::Format("%ld", jumlah));

    lblTotal->SetLabel(wxString::Format("Total Belanja: Rp. %.2f", HitungTotal()));

    txtSearchID->Clear();
    txtJumlah->Clear();
}




void KasirFrame::OnHapusTabel(wxCommandEvent& event) {
    int rowCount = grid->GetNumberRows();
    if (rowCount > 0) {
        grid->DeleteRows(0, rowCount);
    }
    lblTotal->SetLabel("Total Belanja: Rp. 0");
}
void KasirFrame::SimpanTransaksiKeLog(double total, double bayar, double kembalian) {
    // Query untuk menyimpan transaksi
    const char* insertSQL = 
        "INSERT INTO transaksi_log (id_barang, nama_barang, jumlah, total, bayar, kembalian, tanggal) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;

    // Siapkan statement SQL sekali di luar loop
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        wxLogError("Gagal menyiapkan statement SQL: %s", sqlite3_errmsg(db));
        return;
    }

    int rowCount = grid->GetNumberRows();

    for (int i = 0; i < rowCount; ++i) {
        wxString idBarang = grid->GetCellValue(i, 0);
        wxString namaBarang = grid->GetCellValue(i, 1);
        wxString jumlahStr = grid->GetCellValue(i, 3);
        long jumlah;
        if (!jumlahStr.ToLong(&jumlah)) {
            wxLogError("Gagal mengonversi jumlah menjadi angka pada baris %d", i);
            continue;
        }

        // Bind parameter
        sqlite3_bind_text(stmt, 1, idBarang.ToUTF8().data(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, namaBarang.ToUTF8().data(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, static_cast<int>(jumlah));
        sqlite3_bind_double(stmt, 4, total);
        sqlite3_bind_double(stmt, 5, bayar);
        sqlite3_bind_double(stmt, 6, kembalian);

        // Format tanggal menggunakan wxDateTime
        wxDateTime now = wxDateTime::Now();
        wxString tanggal = now.FormatISODate() + " " + now.FormatISOTime();
        sqlite3_bind_text(stmt, 7, tanggal.ToUTF8().data(), -1, SQLITE_STATIC);

        // Eksekusi query
        int result = sqlite3_step(stmt);
        if (result != SQLITE_DONE) {
            wxLogError("Gagal menyimpan transaksi pada baris %d: %s", i, sqlite3_errmsg(db));
        }

        // Reset statement untuk iterasi berikutnya
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    // Finalisasi statement
    sqlite3_finalize(stmt);
}



void KasirFrame::ResetTransaksi() {
    // Clear the grid
    int rowCount = grid->GetNumberRows();
    if (rowCount > 0) {
        grid->DeleteRows(0, rowCount);
    }
    
    // Reset total label
    lblTotal->SetLabel("Total Belanja: Rp. 0");
    
    // Clear input fields
    txtSearchID->Clear();
    txtJumlah->Clear();
    txtBayar->Clear();
}
void KasirFrame::OnProsesTransaksi(wxCommandEvent& event) {
    wxString bayarStr = txtBayar->GetValue();
    double bayar = wxAtof(bayarStr);
    double total = HitungTotal();

    if (bayar < total) {
        wxMessageBox("Jumlah bayar tidak cukup.", "Peringatan", wxOK | wxICON_WARNING);
        return;
    }

    double kembalian = bayar - total;

    wxString kembalianStr = wxString::Format("Kembalian: Rp. %.2f", kembalian);
    wxMessageBox(kembalianStr, "Transaksi Selesai", wxOK | wxICON_INFORMATION);
    // Menyimpan transaksi ke log
    SimpanTransaksiKeLog(total, bayar, kembalian);

    CetakStruk();
        // Call ResetTransaksi to clear all fields and grid after the transaction
    ResetTransaksi();
}

void KasirFrame::SimpanStrukKeFile(const wxString& struk) {
    wxFile file("struk.txt", wxFile::write);
    if (file.IsOpened()) {
        file.Write(struk);
        file.Close();
    } else {
        wxMessageBox("Gagal menyimpan struk ke file.", "Error", wxOK | wxICON_ERROR);
    }
}

void KasirFrame::CetakStruk() {
    wxDateTime now = wxDateTime::Now();
    wxString tanggal = now.FormatISODate() + " " + now.FormatISOTime();

    wxString struk = "=================================\n";
    struk += "=  STRUK TRANSAKSI FJ FOTOCOPY  =\n";
    struk += "=================================\n";
    struk += "Tanggal: " + tanggal + "\n";
    struk += "Barang:\n";

    int rowCount = grid->GetNumberRows();
    for (int i = 0; i < rowCount; ++i) {
        struk += grid->GetCellValue(i, 1) + " - " +
                 grid->GetCellValue(i, 3) + " x " +
                 grid->GetCellValue(i, 2) + "\n";
    }

    struk += "=================================\n";
    struk += "Total: Rp. " + wxString::Format("%.2f", HitungTotal()) + "\n";

    wxString bayarStr = txtBayar->GetValue();
    struk += "Bayar: Rp. " + bayarStr + "\n";

    double kembalian = wxAtof(bayarStr) - HitungTotal();
    struk += "Kembalian: Rp. " + wxString::Format("%.2f", kembalian) + "\n";
    struk += "=================================\n";

    SimpanStrukKeFile(struk);
    wxMessageBox("Transaksi selesai.", "Pemberitahuan", wxOK | wxICON_INFORMATION);
    
        // Cetak struk ke printer
    const char* printer = cupsGetDefault();
    if (printer) {
        cupsPrintFile(printer, "struk.txt", "Struk Transaksi", 0, NULL);
    }
}

KasirFrame::~KasirFrame() {
    sqlite3_close(db); // Close database connection on destruction
}

