#ifndef __MGUI_SDK_TREEMODEL_H__
#define __MGUI_SDK_TREEMODEL_H__

template<typename FieldsT>
struct ColumnRecord: public Singleton<ColumnRecord<FieldsT> >
{
    //typedef ColumnRecord<FieldsT> Type;
    Gtk::TreeModelColumnRecord  rec;
                       FieldsT  flds;

    protected:
    ColumnRecord(): flds(rec) {}
    friend class Singleton<ColumnRecord>; // из-за не тривиального конструктора
};

template<typename FieldsT>
FieldsT& GetColumnFields()
{
    typedef ColumnRecord<FieldsT> Type;
    return Type::Instance().flds;
}

template<typename FieldsT>
Gtk::TreeModelColumnRecord& GetColumnRecord()
{
    typedef ColumnRecord<FieldsT> Type;
    return Type::Instance().rec;
}

#endif // __MGUI_SDK_TREEMODEL_H__

