#ifndef DRIVE_DEDUPLICATOR_FILE_MODEL_H__
#define DRIVE_DEDUPLICATOR_FILE_MODEL_H__

#include <QAbstractItemModel>

#include <hash_db.h>

class FileDataModel: public QAbstractItemModel
	{	
	Q_OBJECT

	public:
		enum Columns {
			HashColumn=0,
			FilePathColumn,

			MAX_COLUMNS
		};

		FileDataModel(HashDb& db, QObject* _parent=NULL);

		virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;
		virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;
		virtual QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const;

		virtual QModelIndex index(int row, int column, const QModelIndex& parent) const;
		virtual QModelIndex parent(const QModelIndex& index) const;

	protected:
		HashDb& database;
	};

#endif // DRIVE_DEDUPLICATOR_FILE_MODEL_H__
