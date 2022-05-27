#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ssdt_index.h"
#include "qmessagebox.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!ssdt_index::LoadNtDLL())
    {
        QMessageBox* dlg = new QMessageBox();
        dlg->setText("ntdll load fail...");
        dlg->exec();
        this->close();
    }

    this->InitTable();
    connect(ui->search_edit, &QLineEdit::textChanged, this, &MainWindow::search_combo_text_changed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::search_combo_text_changed(const QString& text)
{
    int row_count = ui->main_info_table->rowCount();
    for (int idx = 0; idx < row_count; idx++)
        ui->main_info_table->showRow(idx);

    if (!text.isEmpty())
    {
        for (int idx = 0; idx < row_count; idx++)
        {
            auto name = ui->main_info_table->item(idx, 0)->text();
            if (name.contains(text, Qt::CaseInsensitive) == false)
            {
                ui->main_info_table->hideRow(idx);
            }
        }
    }
}

void MainWindow::InitTable()
{
    QStringList col_name_list = {"name", "ssdt index" };
    ui->main_info_table->setColumnCount(col_name_list.length());
    ui->main_info_table->setHorizontalHeaderLabels(col_name_list);

    ui->main_info_table->setColumnWidth(0, 250);
    ui->main_info_table->setColumnWidth(1, 150);

    const auto& function_names = ssdt_index::GetExportFunctions();
    for (int idx = 0, item_idx = 0; idx < function_names.size(); idx++)
    {
        int index = ssdt_index::GetExportSSDTIndex(function_names[idx].c_str());
        if (index == -1) continue;
        if (function_names[idx].find("Nt") != 0
            && function_names[idx].find("Zw") != 0)
            continue;

        int count = ui->main_info_table->rowCount();
        ui->main_info_table->setRowCount(count + 1);

        auto name_item = new QTableWidgetItem(function_names[idx].c_str());
        auto ssdt_index_item = new QTableWidgetItem(QString::number(index));

        ui->main_info_table->setItem(item_idx, 0, name_item);
        ui->main_info_table->setItem(item_idx, 1, ssdt_index_item);
        item_idx++;
    }
}

