#include "AboutDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QUrl>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("About ParquetPad");
    setMinimumSize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();

    // Icon
    QLabel *iconLabel = new QLabel(this);
    QPixmap iconPixmap(":/icons/app_icon.png");
    iconLabel->setPixmap(iconPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    topLayout->addWidget(iconLabel);

    // Text
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->addWidget(new QLabel("<b>ParquetPad</b>", this));
    textLayout->addWidget(new QLabel(QString("Version %1").arg(PARQUETPAD_VERSION), this));
    textLayout->addWidget(new QLabel("Copyright (c) 2025 BiteCat Digital (Pty) Ltd", this));
    
    QLabel *websiteLabel = new QLabel("<a href=\"http://www.bytecat.co.za\">www.bytecat.co.za</a>", this);
    websiteLabel->setOpenExternalLinks(true);
    textLayout->addWidget(websiteLabel);

    QLabel *githubLabel = new QLabel("<a href=\"https://github.com/ByteCat-Digital\">github.com/ByteCat-Digital</a>", this);
    githubLabel->setOpenExternalLinks(true);
    textLayout->addWidget(githubLabel);

    textLayout->addWidget(new QLabel("Built with the help of Google Gemini", this));

    textLayout->addStretch();
    topLayout->addLayout(textLayout);

    mainLayout->addLayout(topLayout);

    // Acknowledgements
    QTextBrowser *acknowledgements = new QTextBrowser(this);
    acknowledgements->setReadOnly(true);
    acknowledgements->setOpenExternalLinks(true);
    acknowledgements->setHtml(
        "This application is built using open-source software. The following is a list of the major components and their licenses:"
        "<br><br>"
        "<b>Qt Framework</b><br>"
        "This application is built with the Qt framework, which is licensed under the GNU Lesser General Public License (LGPL) version 3. "
        "The source code for Qt is available from its website. You can obtain a copy of the Qt source code by visiting "
        "<a href=\"https://www.qt.io/download-open-source\">https://www.qt.io/download-open-source</a>."
        "<br><br>"
        "<b>Apache Arrow</b><br>"
        "This application uses the Apache Arrow library, which is licensed under the Apache License, Version 2.0."
        "<br><br>"
        "<b>ParquetPad</b><br>"
        "ParquetPad itself is licensed under the Apache License, Version 2.0."
    );
    mainLayout->addWidget(acknowledgements);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *viewLicenseButton = new QPushButton("View License", this);
    connect(viewLicenseButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.apache.org/licenses/LICENSE-2.0.txt"));
    });
    buttonLayout->addWidget(viewLicenseButton);

    QPushButton *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

AboutDialog::~AboutDialog() = default;
