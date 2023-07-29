    // For some table widgets, we might have predefined data that is loaded into the
    // table before preferences are restored. Therefore, we want to stop this parent
    // function from calling restorePreferences it gets called twice and therefore
    // have to prune the whole table before restoring preferences. But this pruning
    // then destroys are predefined data. Super annoying.
    // Plz go away: onChangeUpdate (default is true) can be set if to stop this annoyance
    if(!onChangeUpdate) {
        return;
    }


bool DlgSettingsFonts::rowHasSystemPath(int row)
{
    // If not editable, then must be system path
    QTableWidgetItem* cell = ui->pathsTable->item(row, 0);
    if (!cell) {
        return false;
    }

    return !(cell->flags() & Qt::ItemIsEditable);
}




void PrefTableWidget::OnChange(Base::Subject<const char*> &rCaller, const char* sReason)
{
    PrefWidget(rCaller, sReason);
`PrefWidget::restorePreferences`

    // Purpose of onChangeUpdate is avoiding causing issues with predefined data.
    //
    if(onChangeUpdate) {
        setColumnCount(0);
        setRowCount(0);
    }
}




void DlgSettingsFonts::loadSystemPaths()
{
    int row = 0;

    QStringList systemPaths = App::FontPaths::systemPaths();
    for(const QString systemPath : systemPaths) {
        QTableWidgetItem* cell = new QTableWidgetItem(systemPath);
        cell->setFlags(Qt::ItemIsEnabled);
        cell->setToolTip(QString::fromUtf8("System paths cannot be edited"));  // Needs translation
        ui->pathsTable->insertRow(row);
        ui->pathsTable->setItem(row, 0, cell);
        ++row;
    }
}

        // Shouldn't be possible to select a non-selectable cell
        // but there is some bug in Qt when draggin in from non-selectable
        // cells onto selectable cells...
        if(rowHasSystemPath(row)) {
            continue;
        }



    // Remove system paths (we don't want to save systempaths)
    while (rowHasSystemPath(0)) {
        ui->pathsTable->removeRow(0);
    }