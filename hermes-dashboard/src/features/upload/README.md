# Módulo 4: Upload e Arquivos

> **RF-00-FUNCIONAL** — placeholder para implementação pelo produto.

## Descrição

Drag-and-drop, upload múltiplo com barra de progresso por arquivo, validação de tipo MIME e tamanho máximo, tratamento de erros com retry e listagem de arquivos enviados.

## Estrutura esperada

```
src/features/upload/
├── index.ts
├── components/        # Dropzone, FileList, FileItem, UploadProgress
├── hooks/             # useUpload, useFileValidation
├── services/          # upload.api.ts — presigned URLs ou multipart
├── types.ts           # UploadFile, UploadStatus, FileValidationRules
└── README.md
```

## Referências

- Spec: `RF-00-FUNCIONAL.md` — Módulo 4
