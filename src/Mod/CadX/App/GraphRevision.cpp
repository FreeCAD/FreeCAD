// SPDX-License-Identifier: LGPL-2.1-or-later

#include "GraphRevision.h"

#include "GraphSnapshot.h"

#include <QByteArray>
#include <QCryptographicHash>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace CadX
{
namespace
{
std::string escape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (char character : value) {
        switch (character) {
            case '\\': result += "\\\\"; break;
            case '|': result += "\\|"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            default: result += character; break;
        }
    }
    return result;
}

std::string geometry(const GeometrySummary& value)
{
    std::ostringstream stream;
    stream << value.available << '|' << value.valid << '|' << escape(value.kind) << '|'
           << value.solids << '|' << value.shells << '|' << value.faces << '|' << value.edges
           << '|' << value.vertices << '|' << std::setprecision(17) << value.volume << '|'
           << value.area << '|' << escape(value.signature);
    return stream.str();
}

std::string payload(const NodePayload& value)
{
    return std::visit(
        [](const auto& item) -> std::string {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            }
            else if constexpr (std::is_same_v<T, DefinitionPayload>) {
                return "definition|" + escape(item.role) + "|" + escape(item.containerKind)
                    + "|" + escape(item.geometryKind) + "|" + escape(item.provenanceKind)
                    + "|" + escape(item.semanticPartKind);
            }
            else if constexpr (std::is_same_v<T, OccurrencePayload>) {
                std::string result = "occurrence|" + std::to_string(item.rigid) + "|"
                    + std::to_string(item.flexible) + "|" + geometry(item.geometry);
                for (const auto& path : item.occurrencePath) {
                    result += "|" + escape(path);
                }
                return result;
            }
            else if constexpr (std::is_same_v<T, RelationPayload>) {
                return "relation|" + escape(item.relationType) + "|"
                    + std::to_string(item.suppressed);
            }
            else if constexpr (std::is_same_v<T, ArtifactPayload>) {
                return "artifact|" + escape(item.artifactType);
            }
            else if constexpr (std::is_same_v<T, PrimitivePayload>) {
                std::ostringstream result;
                result << "primitive|" << escape(item.primitiveKind) << '|'
                       << std::setprecision(17) << item.length << '|' << item.width << '|'
                       << item.height << '|' << item.radius << '|' << item.sweepDegrees;
                return result.str();
            }
            else if constexpr (std::is_same_v<T, JointPayload>) {
                const auto connector = [](const JointConnectorPayload& item) {
                    auto offset = item.offset;
                    offset.normalize();
                    return escape(item.componentObject) + "|" + escape(item.connectorType) + "|"
                        + escape(item.connector) + "|" + std::to_string(item.hasOffset) + "|"
                        + offset.canonical();
                };
                std::ostringstream result;
                result << "joint|" << escape(item.jointType) << '|' << item.reverse << '|'
                       << item.hasLimits << '|' << std::setprecision(17) << item.minDegrees
                       << '|' << item.maxDegrees << '|' << connector(item.first) << '|'
                       << connector(item.second);
                return result.str();
            }
            else if constexpr (std::is_same_v<T, GroundConstraintPayload>) {
                return "ground_constraint|" + std::to_string(item.grounded) + '|'
                    + escape(item.constrainedObject) + '|' + escape(item.groundedJointObject);
            }
            else {
                return "unresolved|" + escape(item.requestedDocument) + "|"
                    + escape(item.requestedObject) + "|" + escape(item.diagnostic);
            }
        },
        value);
}

}  // namespace

std::string canonicalSemantic(const GraphSnapshot& snapshot)
{
    std::vector<const NodeRecord*> nodes;
    nodes.reserve(snapshot.nodes().size());
    for (const auto& node : snapshot.nodes()) {
        nodes.push_back(&node);
    }
    std::sort(nodes.begin(), nodes.end(), [](const NodeRecord* left, const NodeRecord* right) {
        return left->id < right->id;
    });

    std::vector<const EdgeRecord*> edges;
    edges.reserve(snapshot.edges().size());
    for (const auto& edge : snapshot.edges()) {
        edges.push_back(&edge);
    }
    std::sort(edges.begin(), edges.end(), [](const EdgeRecord* left, const EdgeRecord* right) {
        if (left->kind != right->kind) {
            return static_cast<int>(left->kind) < static_cast<int>(right->kind);
        }
        if (left->from != right->from) {
            return left->from < right->from;
        }
        if (left->to != right->to) {
            return left->to < right->to;
        }
        return left->id < right->id;
    });

    std::ostringstream result;
    result << "cadx.graph.v2|document=" << escape(snapshot.header().documentUid) << '|';
    for (const auto* node : nodes) {
        result << "N|" << escape(node->id) << '|' << nodeKindName(node->kind) << '|'
               << escape(node->native.canonical()) << '|' << escape(node->display.label) << '|'
               << escape(node->display.normalizedLabel)
               << '|' << escape(node->provenance.kind) << '|' << node->localPlacement.canonical()
               << '|' << node->worldPlacement.canonical() << '|' << geometry(node->geometry) << '|'
               << node->unresolved << '|' << payload(node->payload) << '\n';
    }
    for (const auto* edge : edges) {
        result << "E|" << escape(edge->id) << '|' << edgeKindName(edge->kind) << '|'
               << escape(edge->from) << '|' << escape(edge->to) << '|'
               << escape(edge->provenance.kind) << '|'
               << escape(edge->provenance.evidence) << '|' << escape(edge->relation) << '\n';
    }
    return result.str();
}

std::string canonicalPresentation(const GraphSnapshot& snapshot)
{
    std::vector<const NodeRecord*> nodes;
    nodes.reserve(snapshot.nodes().size());
    for (const auto& node : snapshot.nodes()) {
        nodes.push_back(&node);
    }
    std::sort(nodes.begin(), nodes.end(), [](const NodeRecord* left, const NodeRecord* right) {
        return left->id < right->id;
    });
    std::ostringstream result;
    result << "cadx.presentation.v2|view=" << escape(snapshot.header().activeViewId) << '|';
    for (const auto* node : nodes) {
        result << escape(node->id) << '|' << escape(node->display.label) << '|'
               << node->presentation.visible << '|'
               << node->presentation.selected << '|' << escape(node->presentation.viewId) << '\n';
    }
    result << "camera=" << escape(snapshot.header().cameraState) << '\n';
    return result.str();
}

std::string sha256Revision(const std::string& canonical)
{
    const QByteArray digest = QCryptographicHash::hash(
        QByteArray::fromStdString(canonical), QCryptographicHash::Sha256);
    return "sha256:" + digest.toHex().toStdString();
}

}  // namespace CadX
