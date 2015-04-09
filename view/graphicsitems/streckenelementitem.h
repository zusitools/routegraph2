#ifndef STRECKENELEMENTITEM_H
#define STRECKENELEMENTITEM_H

#include <zusi_file_lib/src/model/streckenelement.hpp>

#include <QGraphicsLineItem>

using namespace std;

class StreckenelementItem : public QGraphicsLineItem
{
public:
    explicit StreckenelementItem(QGraphicsItem *parent = 0, shared_ptr<Streckenelement> streckenelement = nullptr);

private:
    shared_ptr<Streckenelement> m_streckenelement;
};

#endif // STRECKENELEMENTITEM_H
