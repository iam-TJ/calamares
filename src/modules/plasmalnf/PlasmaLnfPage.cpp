/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2017-2018 Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2019 Collabora Ltd <arnaud.ferraris@collabora.com>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "PlasmaLnfPage.h"

#include "ui_page_plasmalnf.h"

#include "Settings.h"
#include "utils/Logger.h"
#include "utils/Retranslator.h"

#include <QAbstractButton>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

static ThemeInfoList
plasma_themes()
{
    ThemeInfoList packages;

    QList< KPluginMetaData > pkgs = KPackage::PackageLoader::self()->listPackages( "Plasma/LookAndFeel" );

    for ( const KPluginMetaData& data : pkgs )
    {
        if ( data.isValid() && !data.isHidden() && !data.name().isEmpty() )
        {
            packages << ThemeInfo { data };
        }
    }

    return packages;
}


PlasmaLnfPage::PlasmaLnfPage( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::PlasmaLnfPage )
    , m_showAll( false )
    , m_buttonGroup( nullptr )
{
    ui->setupUi( this );
    CALAMARES_RETRANSLATE( {
        ui->retranslateUi( this );
        if ( Calamares::Settings::instance()->isSetupMode() )
            ui->generalExplanation->setText( tr( "Please choose a look-and-feel for the KDE Plasma Desktop. "
                                                 "You can also skip this step and configure the look-and-feel "
                                                 "once the system is set up. Clicking on a look-and-feel "
                                                 "selection will give you a live preview of that look-and-feel." ) );
        else
            ui->generalExplanation->setText( tr( "Please choose a look-and-feel for the KDE Plasma Desktop. "
                                                 "You can also skip this step and configure the look-and-feel "
                                                 "once the system is installed. Clicking on a look-and-feel "
                                                 "selection will give you a live preview of that look-and-feel." ) );
        updateThemeNames();
        fillUi();
    } )
}

void
PlasmaLnfPage::setEnabledThemes( const ThemeInfoList& themes, bool showAll )
{
    m_enabledThemes = themes;

    if ( showAll )
    {
        auto plasmaThemes = plasma_themes();
        for ( auto& installed_theme : plasmaThemes )
            if ( !m_enabledThemes.findById( installed_theme.id ) )
            {
                m_enabledThemes.append( installed_theme );
            }
    }

    updateThemeNames();
    winnowThemes();
    fillUi();
}

void
PlasmaLnfPage::setEnabledThemesAll()
{
    // Don't need to set showAll=true, because we're already passing in
    // the complete list of installed themes.
    setEnabledThemes( plasma_themes(), false );
}

void
PlasmaLnfPage::setPreselect( const QString& id )
{
    m_preselect = id;
    if ( !m_enabledThemes.isEmpty() )
    {
        fillUi();
    }
}

void
PlasmaLnfPage::updateThemeNames()
{
    auto plasmaThemes = plasma_themes();
    for ( auto& enabled_theme : m_enabledThemes )
    {
        ThemeInfo* t = plasmaThemes.findById( enabled_theme.id );
        if ( t != nullptr )
        {
            enabled_theme.name = t->name;
            enabled_theme.description = t->description;
        }
    }
}

void
PlasmaLnfPage::winnowThemes()
{
    auto plasmaThemes = plasma_themes();
    bool winnowed = true;
    int winnow_index = 0;
    while ( winnowed )
    {
        winnowed = false;
        winnow_index = 0;

        for ( auto& enabled_theme : m_enabledThemes )
        {
            ThemeInfo* t = plasmaThemes.findById( enabled_theme.id );
            if ( t == nullptr )
            {
                cDebug() << "Removing" << enabled_theme.id;
                winnowed = true;
                break;
            }
            ++winnow_index;
        }

        if ( winnowed )
        {
            m_enabledThemes.removeAt( winnow_index );
        }
    }
}

void
PlasmaLnfPage::fillUi()
{
    if ( m_enabledThemes.isEmpty() )
    {
        return;
    }

    if ( !m_buttonGroup )
    {
        m_buttonGroup = new QButtonGroup( this );
        m_buttonGroup->setExclusive( true );
    }

    int c = 1;  // After the general explanation
    for ( auto& theme : m_enabledThemes )
    {
        if ( !theme.widget )
        {
            ThemeWidget* w = new ThemeWidget( theme );
            m_buttonGroup->addButton( w->button() );
            ui->verticalLayout->insertWidget( c, w );
            connect( w, &ThemeWidget::themeSelected, this, &PlasmaLnfPage::plasmaThemeSelected );
            theme.widget = w;
        }
        else
        {
            theme.widget->updateThemeName( theme );
        }
        if ( theme.id == m_preselect )
        {
            const QSignalBlocker b( theme.widget->button() );
            theme.widget->button()->setChecked( true );
        }
        ++c;
    }
}
