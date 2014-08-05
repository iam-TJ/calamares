/* === This file is part of Calamares - <http://github.com/calamares> ===
 *
 *   Copyright 2014, Aurélien Gâteau <agateau@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

// This class is heavily based on the ResizeOperation class from KDE Partition
// Manager. Original copyright follow:

/***************************************************************************
 *   Copyright (C) 2008,2012 by Volker Lanz <vl@fidra.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <ResizePartitionJob.h>

//#include <utils/Logger.h>

// CalaPM
#include <backend/corebackend.h>
#include <backend/corebackendmanager.h>
#include <backend/corebackenddevice.h>
#include <backend/corebackendpartition.h>
#include <backend/corebackendpartitiontable.h>
#include <core/device.h>
#include <core/partition.h>
/*
#include <core/partitiontable.h>
#include <fs/filesystem.h>
*/
#include <util/report.h>

// Qt
#include <QScopedPointer>

//- Context --------------------------------------------------------------------
struct Context
{
    Context( ResizePartitionJob* job_ )
        : job( job_ )
    {}

    ResizePartitionJob* job;
};

//- ResizeFileSystemJob --------------------------------------------------------
class ResizeFileSystemJob : public Calamares::Job
{
public:
    ResizeFileSystemJob( Context* context, qint64 length )
    {}

    QString prettyName() const override
    {
        return QString();
    }

    Calamares::JobResult exec() override
    {
        return Calamares::JobResult::ok();
    }
};

//- SetPartGeometryJob ---------------------------------------------------------
class SetPartGeometryJob : public Calamares::Job
{
public:
    SetPartGeometryJob( Context* context, qint64 firstSector, qint64 length )
    {}

    QString prettyName() const override
    {
        return QString();
    }

    Calamares::JobResult exec() override
    {
        return Calamares::JobResult::ok();
    }
};

//- MoveFileSystemJob ----------------------------------------------------------
class MoveFileSystemJob : public Calamares::Job
{
public:
    MoveFileSystemJob( Context* context, qint64 firstSector )
    {}

    QString prettyName() const override
    {
        return QString();
    }

    Calamares::JobResult exec() override
    {
        return Calamares::JobResult::ok();
    }
};

//- ResizePartitionJob ---------------------------------------------------------
ResizePartitionJob::ResizePartitionJob( Device* device, Partition* partition, qint64 firstSector, qint64 lastSector )
    : PartitionJob( partition )
    , m_device( device )
    , m_newFirstSector( firstSector )
    , m_newLastSector( lastSector )
{
}

QString
ResizePartitionJob::prettyName() const
{
    /*
    return tr( "Format partition %1 (file system: %2, size: %3 MB) on %4." )
           .arg( m_partition->partitionPath() )
           .arg( m_partition->fileSystem().name() )
           .arg( m_partition->capacity() / 1024 / 1024 )
           .arg( m_device->name() );
    */
    return QString();
}

Calamares::JobResult
ResizePartitionJob::exec()
{
    /*
    Report report( 0 );
    QString partitionPath = m_partition->partitionPath();
    QString message = tr( "The installer failed to format partition %1 on disk '%2'." ).arg( partitionPath, m_device->name() );

    CoreBackend* backend = CoreBackendManager::self()->backend();
    QScopedPointer<CoreBackendDevice> backendDevice( backend->openDevice( m_device->deviceNode() ) );
    if ( !backendDevice.data() )
    {
        return Calamares::JobResult::error(
                   message,
                   tr( "Could not open device '%1'." ).arg( m_device->deviceNode() )
               );
    }

    QScopedPointer<CoreBackendPartitionTable> backendPartitionTable( backendDevice->openPartitionTable() );
    backendPartitionTable->commit();
    return Calamares::JobResult::ok();
    */
    qint64 oldLength = m_partition->lastSector() - m_partition->firstSector() + 1;
    qint64 newLength = m_newLastSector - m_newFirstSector + 1;

    Context context( this );
    QList< Calamares::job_ptr > jobs;
    if ( m_partition->roles().has( PartitionRole::Extended ) )
        jobs << Calamares::job_ptr( new SetPartGeometryJob( &context, m_newFirstSector, newLength ) );
    else
    {
        bool shrink = newLength < oldLength;
        bool grow = newLength > oldLength;
        bool moveRight = m_newFirstSector > m_partition->firstSector();
        bool moveLeft = m_newFirstSector < m_partition->firstSector();
        if ( shrink )
        {
            jobs << Calamares::job_ptr( new ResizeFileSystemJob( &context, newLength ) );
            jobs << Calamares::job_ptr( new SetPartGeometryJob( &context, m_partition->firstSector(), newLength ) );
        }
        if ( moveRight || moveLeft )
        {
            // At this point, we need to set the partition's length to either the resized length, if it has already been
            // shrunk, or to the original length (it may or may not then later be grown, we don't care here)
            const qint64 length = shrink ? newLength : oldLength;
            jobs << Calamares::job_ptr( new SetPartGeometryJob( &context, m_newFirstSector, length ) );
            jobs << Calamares::job_ptr( new MoveFileSystemJob( &context, m_newFirstSector ) );
        }
        if ( grow )
        {
            jobs << Calamares::job_ptr( new SetPartGeometryJob( &context, m_newFirstSector, newLength ) );
            jobs << Calamares::job_ptr( new ResizeFileSystemJob( &context, newLength ) );
        }
    }
    return execJobList( jobs );
}

void
ResizePartitionJob::updatePreview()
{
    m_device->partitionTable()->removeUnallocated();
    m_partition->parent()->remove( m_partition );
    m_partition->setFirstSector( m_newFirstSector );
    m_partition->setLastSector( m_newLastSector );
    m_partition->parent()->insert( m_partition );
    m_device->partitionTable()->updateUnallocated( *m_device );
}

Calamares::JobResult
ResizePartitionJob::execJobList( const QList< Calamares::job_ptr >& jobs )
{
    int nbJobs = jobs.size();
    int count = 0;
    for ( Calamares::job_ptr job : jobs )
    {
        Calamares::JobResult result = job->exec();
        if ( !result )
            return result;
        ++count;
        progress( qreal( count ) / nbJobs );
    }
    return Calamares::JobResult::ok();
}
