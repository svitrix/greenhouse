import { useMutation } from "@tanstack/react-query";
import { apiFetch } from "@/lib/api";
import {
  type LoginFormValues,
  type LoginResponse,
  loginResponseSchema,
} from "@/schemas/auth";

export function useLoginMutation() {
  return useMutation({
    mutationFn: async (values: LoginFormValues): Promise<LoginResponse> => {
      const raw = await apiFetch<unknown>("/api/auth/login", {
        method: "POST",
        body: JSON.stringify(values),
      });
      return loginResponseSchema.parse(raw);
    },
  });
}
